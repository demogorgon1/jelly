#include <jelly/API.h>

#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{
		
		namespace 
		{
			
			void
			_LockNodeReplication()
			{
				DefaultHost host(".", "reptest", NULL);
				host.DeleteAllFiles(UINT32_MAX);

				typedef MetaData::LockStaticSingleBlob<1> LockMetaDataType;
				typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType> LockNodeType;

				LockNodeType node0(&host, 0);
				LockNodeType node1(&host, 1);

				ReplicationNetwork node0Replication(0);
				node0Replication.SetSendCallback([&node1](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ALWAYS_ASSERT(aNodeId == 1);
					JELLY_ALWAYS_ASSERT(node1.ProcessReplication(0, aHead) == 1);
					node1.FlushPendingWAL();
				});
				node0.SetReplicationNetwork(&node0Replication);
					
				ReplicationNetwork node1Replication(1);
				node1Replication.SetSendCallback([&node0](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ALWAYS_ASSERT(aNodeId == 0);
					JELLY_ALWAYS_ASSERT(node0.ProcessReplication(1, aHead) == 1);
					node0.FlushPendingWAL();
				});
				node1.SetReplicationNetwork(&node1Replication);

				// Initially node 0 is master, node 1 is slave
				node0Replication.SetNodeIds({ 0, 1 });
				node1Replication.SetNodeIds({ 0, 1 });

				// Check that we can't submit requests to non-master node
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node1.Lock(&req);
					node1.ProcessRequests();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_NOT_MASTER);
				}

				// Submit valid lock request to master
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node0.Lock(&req);

					node0.ProcessRequests();
					node0.FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);

					node0Replication.Update();
				}

				// Switch master
				node0Replication.SetNodeIds({ 1, 0 });
				node1Replication.SetNodeIds({ 1, 0 });

				// Try to unlock on previous master, should fail
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node0.Unlock(&req);
					node0.ProcessRequests();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_NOT_MASTER);
				}

				// Unlock on previous slave, should have the lock
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node1.Unlock(&req);

					node1.ProcessRequests();
					node1.FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);

					node1Replication.Update();
				}

				// Switch master again
				node0Replication.SetNodeIds({ 0, 1 });
				node1Replication.SetNodeIds({ 0, 1 });

				// Try to lock with new lock id on master, should work
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(2000);
					node0.Lock(&req);

					node0.ProcessRequests();
					node0.FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);
				}
			}

			void
			_BlobNodeReplication(
				bool		aMemoryLimit)
			{
				DefaultConfigSource config;
				
				if(aMemoryLimit)
					config.Set(Config::ID_MAX_RESIDENT_BLOB_SIZE, "0");

				DefaultHost host(".", "reptest", &config);
				host.DeleteAllFiles(UINT32_MAX);

				typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;

				std::unique_ptr<BlobNodeType> node0 = std::make_unique<BlobNodeType>(&host, 0);
				std::unique_ptr<BlobNodeType> node1 = std::make_unique<BlobNodeType>(&host, 1);

				ReplicationNetwork node0Replication(0);
				node0Replication.SetSendCallback([&node1](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ALWAYS_ASSERT(aNodeId == 1);
					JELLY_ALWAYS_ASSERT(node1->ProcessReplication(0, aHead) == 1);
					node1->FlushPendingWAL();
				});
				node0->SetReplicationNetwork(&node0Replication);
					
				ReplicationNetwork node1Replication(1);
				node1Replication.SetSendCallback([&node0](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ALWAYS_ASSERT(aNodeId == 0);
					JELLY_ALWAYS_ASSERT(node0->ProcessReplication(1, aHead) == 1);
					node0->FlushPendingWAL();
				});
				node1->SetReplicationNetwork(&node1Replication);

				// Initially node 0 is master, node 1 is slave
				node0Replication.SetNodeIds({ 0, 1 });
				node1Replication.SetNodeIds({ 0, 1 });

				// Write blob to master (already checked negative code path for lock nodes, it's the same)
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetBlob(new UInt32Blob(1000));
					req.SetSeq(1);
					node0->Set(&req);
					node0->ProcessRequests();
					node0->FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);

					node0Replication.Update();
				}

				// Switch master
				node0Replication.SetNodeIds({ 1, 0 });
				node1Replication.SetNodeIds({ 1, 0 });

				// Read blob from previous slave
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(1);
					node1->Get(&req);
					node1->ProcessRequests();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);
					JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1000);
				}

				// Write to previous slave
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetBlob(new UInt32Blob(1001));
					req.SetSeq(2);
					node1->Set(&req);
					node1->ProcessRequests();
					node1->FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);

					node1Replication.Update();
				}

				// Switch back to original master
				node0Replication.SetNodeIds({ 0, 1 });
				node1Replication.SetNodeIds({ 0, 1 });

				// Restart original master
				node0.reset();
				node0 = std::make_unique<BlobNodeType>(&host, 0);

				// Read blob from original master
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(2);
					node0->Get(&req);
					node0->ProcessRequests();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);
					JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1001);
				}
			}

		}

		namespace ReplicationTest
		{			

			void   
			Run()
			{
				_LockNodeReplication();

				_BlobNodeReplication(true); // With memory limit
				_BlobNodeReplication(false); // Without memory limit
			}

		}
	}

}