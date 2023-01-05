#include <jelly/BlobNode.h>
#include <jelly/LockNode.h>
#include <jelly/UIntKey.h>
#include <jelly/UIntLock.h>

#include "Config.h"
#include "MemoryHost.h"
#include "StepThread.h"
#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{

		namespace
		{
			typedef BlobNode<
				UIntKey<uint32_t>,
				UInt32Blob,
				UIntKey<uint32_t>::Hasher> BlobNodeType;

			typedef LockNode<
				UIntKey<uint32_t>,
				UIntLock<uint32_t>,
				UIntKey<uint32_t>::Hasher> LockNodeType;

			typedef StepThread<LockNodeType, LockNodeType::Request, BlobNodeType, BlobNodeType::Request> StepThreadType;

			StepThreadType*
			_LockNodeStepThread(
				std::unique_ptr<LockNodeType>&	aLockNode,
				std::unique_ptr<BlobNodeType>&	aBlobNode,
				std::mt19937&					aRandom,
				IHost*							aHost)
			{
				std::unique_ptr<StepThreadType> t = std::make_unique<StepThreadType>("lock", aHost, aLockNode, aBlobNode, aRandom);

				t->SetVerbose(false);

				t->RestartLockNode();
				t->ProcessLockNodeRequests();
				t->FlushPendingLockNodeWAL();
				t->FlushPendingLockNodeStore();
				t->CleanupLockNodeWALs();

				return t.release();
			}

			StepThreadType*
			_BlobNodeStepThread(
				std::unique_ptr<LockNodeType>&	aLockNode,
				std::unique_ptr<BlobNodeType>&	aBlobNode,
				std::mt19937&					aRandom,
				IHost*							aHost)
			{
				std::unique_ptr<StepThreadType> t = std::make_unique<StepThreadType>("blob", aHost, aLockNode, aBlobNode, aRandom);

				t->SetVerbose(false);

				t->RestartBlobNode(1024 * 1024);
				t->ProcessBlobNodeRequests();
				t->FlushPendingBlobNodeWAL();
				t->FlushPendingBlobNodeStore();
				t->CleanupBlobNodeWALs();

				return t.release();
			}

			StepThreadType*
			_GameServerStepThread(
				uint32_t						aGameServerIndex,
				std::unique_ptr<LockNodeType>&	aLockNode,
				std::unique_ptr<BlobNodeType>&	aBlobNode,
				std::mt19937&					aRandom,
				IHost*							aHost)
			{				
				char threadName[128];
				sprintf(threadName, "server %u", aGameServerIndex);

				std::unique_ptr<StepThreadType> t = std::make_unique<StepThreadType>(threadName, aHost, aLockNode, aBlobNode, aRandom);

				t->SetVerbose(false);

				t->SetLockId(1000 + aGameServerIndex);
				t->SetKey(100 + aRandom() % 10);

				t->Lock();
				t->Get();

				for(uint32_t i = 0; i < 5; i++)
					t->Set();

				t->Unlock();

				return t.release();
			}

			void
			_RunTestWithSeed(
				uint32_t						aSeed,
				uint32_t						aStepCount)
			{
				std::mt19937 random(aSeed);

				MemoryHost host;
				
				std::unique_ptr<LockNodeType> lockNode;
				std::unique_ptr<BlobNodeType> blobNode;

				std::vector<std::unique_ptr<StepThreadType>> threads;
				threads.push_back(std::unique_ptr<StepThreadType>(_LockNodeStepThread(lockNode, blobNode, random, &host)));
				threads.push_back(std::unique_ptr<StepThreadType>(_BlobNodeStepThread(lockNode, blobNode, random, &host)));

				for(uint32_t i = 0; i < 10; i++)
					threads.push_back(std::unique_ptr<StepThreadType>(_GameServerStepThread(i, lockNode, blobNode, random, &host)));

				for(uint32_t i = 0; i < aStepCount; i++)
				{
					size_t randomThreadIndex = (size_t)random() % threads.size();

					threads[randomThreadIndex]->Step(i);
				}

				// Release nodes first because this will cause requests (memory) held by threads to be canceled
				lockNode.reset();
				blobNode.reset();
			}
		}

		namespace StepTest
		{

			void		
			Run(
				const Config*		aConfig)
			{	
				uint32_t stepTestSeed = (uint32_t)time(NULL);

				if(aConfig->m_stepTestSeed != 0)
					stepTestSeed = aConfig->m_stepTestSeed;

				std::mt19937 seedGenerator(stepTestSeed);

				for(uint32_t i = 0; i < 10; i++)
					_RunTestWithSeed(seedGenerator(), 10000);
			}

		}

	}

}