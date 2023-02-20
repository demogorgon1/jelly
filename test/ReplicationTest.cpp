#include <jelly/API.h>

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

				typedef LockMetaData::StaticSingleBlob<1> LockMetaDataType;
				typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType> LockNodeType;

				LockNodeType node0(&host, 0);
				LockNodeType node1(&host, 1);

				ReplicationNetwork node0Replication(0);
				node0Replication.SetSendCallback([&node1](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ASSERT(aNodeId == 1);
					node1.ProcessReplication(0, aHead);
				});
				node0.SetReplicationNetwork(&node0Replication);
					
				ReplicationNetwork node1Replication(1);
				node1Replication.SetSendCallback([&node0](
					uint32_t				aNodeId,
					const Stream::Buffer*	aHead)
				{
					JELLY_ASSERT(aNodeId == 0);
					node0.ProcessReplication(1, aHead);				
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
					JELLY_ASSERT(req.IsCompleted());
					JELLY_ASSERT(req.GetResult() == RESULT_NOT_MASTER);
				}

				// Submit valid lock request to master
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node0.Lock(&req);

					node0.ProcessRequests();
					node0.FlushPendingWAL();
					JELLY_ASSERT(req.IsCompleted());
					JELLY_ASSERT(req.GetResult() == RESULT_OK);

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
					JELLY_ASSERT(req.IsCompleted());
					JELLY_ASSERT(req.GetResult() == RESULT_NOT_MASTER);
				}

				// Unlock on previous slave, should have the lock
				{
					LockNodeType::Request req;
					req.SetKey(123);
					req.SetLock(1000);
					node1.Unlock(&req);

					node1.ProcessRequests();
					node1.FlushPendingWAL();
					JELLY_ASSERT(req.IsCompleted());
					JELLY_ASSERT(req.GetResult() == RESULT_OK);

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
					JELLY_ASSERT(req.IsCompleted());
					JELLY_ASSERT(req.GetResult() == RESULT_OK);
				}
			}
		}

		namespace ReplicationTest
		{			

			void   
			Run()
			{
				_LockNodeReplication();
			}

		}
	}

}