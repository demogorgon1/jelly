// Big pile of node testing mess. Not making any allusions about this providing full covering of everything,
// as it's more of a "shotgun" approach to testing. It covers a lot of stuff, but definetely not all.

#include <optional>
#include <random>
#include <thread>
#include <unordered_set>

#include <jelly/API.h>

#include "Config.h"
#include "NodeTest.h"
#include "TestDefaultHost.h"
#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{
		
		namespace
		{

			typedef BlobNode<UIntKey<uint32_t>, UInt32Blob, UIntKey<uint32_t>::Hasher> BlobNodeType;
			typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, UIntKey<uint32_t>::Hasher> LockNodeType;

			typedef CompactionResult<UIntKey<uint32_t>, UIntKey<uint32_t>::Hasher> CompactionResultType;

			struct BlobNodeItemData
			{
				bool
				CompareItem(
					const Compression::IProvider*							aCompression,
					const BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>&		aItem) const
				{
					UInt32Blob blob;
					blob.FromBuffer(aCompression, *aItem.m_blob);
					return m_key == aItem.m_key && m_seq == aItem.m_meta.m_seq && m_blob == blob;
				}

				// Public data
				uint32_t	m_key;
				uint32_t	m_seq;
				uint32_t	m_blob;
			};

			template <typename _NodeType>
			void
			_PerformCompaction(
				_NodeType*													aNode,
				IHost*														aHost)
			{
				CompactionAdvisor compactionAdvisor(aNode->GetNodeId(), aHost, 1, 1, 0, CompactionAdvisor::STRATEGY_SIZE_TIERED);
				compactionAdvisor.Update();

				CompactionJob compactionJob = compactionAdvisor.GetNextSuggestion();
				if(compactionJob.IsSet())
				{					
					std::unique_ptr<CompactionResultType> compactionResult(aNode->PerformCompaction(compactionJob));
					aNode->ApplyCompactionResult(compactionResult.get());
				}
			}

			template <typename _ItemType>
			void
			_VerifyLockNodeFileStreamReader(
				Compression::IProvider*										/*aCompression*/,
				IFileStreamReader*											aFileStreamReader,
				const std::vector<_ItemType>&								aExpected)
			{
				std::unique_ptr<IFileStreamReader> f(aFileStreamReader);
				JELLY_ASSERT(f);

				for (const _ItemType& expected : aExpected)
				{
					JELLY_ASSERT(!f->IsEnd());
					_ItemType item;
					JELLY_ASSERT(item.Read(f.get(), NULL));
					JELLY_ASSERT(item.Compare(&expected));
				}

				JELLY_ASSERT(f->IsEnd());
			}

			template <typename _ItemType>
			void
			_VerifyBlobNodeFileStreamReader(
				Compression::IProvider*										aCompression,
				IFileStreamReader*											aFileStreamReader,
				const std::vector<BlobNodeItemData>&						aExpected)
			{
				std::unique_ptr<IFileStreamReader> f(aFileStreamReader);
				JELLY_ASSERT(f);

				for (const BlobNodeItemData& expected : aExpected)
				{
					JELLY_ASSERT(!f->IsEnd());
					_ItemType item;
					JELLY_ASSERT(item.Read(f.get(), NULL));

					JELLY_ASSERT(expected.CompareItem(aCompression, item));
				}

				JELLY_ASSERT(f->IsEnd());
			}

			template <typename _ItemType>
			void
			_VerifyLockNodeWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<_ItemType>&								aExpected)
			{
				_VerifyLockNodeFileStreamReader<_ItemType>(NULL, aHost->ReadWALStream(aNodeId, aId, true), aExpected);
			}

			template <typename _ItemType>
			void
			_VerifyBlobNodeWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<BlobNodeItemData>&						aExpected)
			{
				_VerifyBlobNodeFileStreamReader<_ItemType>(NULL, aHost->ReadWALStream(aNodeId, aId, false), aExpected);
			}

			template <typename _ItemType>
			void
			_VerifyLockNodeStore(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<_ItemType>&								aExpected)
			{
				_VerifyLockNodeFileStreamReader<_ItemType>(aHost->GetCompressionProvider(), aHost->ReadStoreStream(aNodeId, aId), aExpected);
			}

			template <typename _ItemType>
			void
			_VerifyBlobNodeStore(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<BlobNodeItemData>&						aExpected)
			{
				_VerifyBlobNodeFileStreamReader<_ItemType>(aHost->GetCompressionProvider(), aHost->ReadStoreStream(aNodeId, aId), aExpected);
			}

			void
			_VerifyNoWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId)
			{
				std::unique_ptr<IFileStreamReader> f(aHost->ReadWALStream(aNodeId, aId, false));
				JELLY_ASSERT(!f);
			}

			void
			_VerifyNoStore(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId)
			{
				std::unique_ptr<IFileStreamReader> f(aHost->ReadStoreStream(aNodeId, aId));
				JELLY_ASSERT(!f);
			}

			template <typename _NodeType>
			void
			_VerifyResidentKeys(
				_NodeType*													aNode,
				const std::vector<UIntKey<uint32_t>>&						aExpected)
			{
				std::vector<UIntKey<uint32_t>> keys;
				aNode->GetResidentKeys(keys);
				JELLY_ASSERT(keys == aExpected);
			}

			void
			_HammerTestThreadGeneral(
				LockNodeType*			aLockNode,
				BlobNodeType*			aBlobNode,
				std::atomic_bool*		aStopEvent,
				uint32_t				aLockId,
				uint32_t				aKey,
				std::atomic_int*		aLockCounter,
				std::atomic_int*		aSetCounter,
				std::atomic_int*		aGetCounter,
				std::atomic_int*		aThreadCounter)
			{
				while(!*aStopEvent)
				{	
					uint32_t blobSeq = UINT32_MAX;
					uint32_t blobNodeId = UINT32_MAX;

					// Try to lock
					{
						LockNodeType::Request req;
						req.m_key = aKey;
						req.m_lock = aLockId;
						aLockNode->Lock(&req);
						while(!req.m_completed.Poll())
							std::this_thread::yield();
						if(req.m_result == RESULT_ALREADY_LOCKED)
							continue;
						JELLY_ASSERT(req.m_result == RESULT_OK);
						blobSeq = req.m_blobSeq;
						if(req.m_blobNodeIds != UINT32_MAX)
						{	
							blobNodeId = req.m_blobNodeIds & 0x000000FF;
						}

						(*aLockCounter)++;
					}

					// If we have a blob, get it
					if(blobSeq != UINT32_MAX)
					{
						JELLY_ASSERT(blobNodeId == 1);

						BlobNodeType::Request req;
						req.m_key = aKey;
						req.m_seq = blobSeq;
						aBlobNode->Get(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == aKey);

						blobSeq = req.m_seq;

						(*aGetCounter)++;
					}
					else
					{
						blobSeq = 0;
					}

					// Do a bunch of sets
					for(uint32_t i = 0; i < 10; i++)
					{
						BlobNodeType::Request req;
						req.m_key = aKey;
						req.m_seq = ++blobSeq;
						req.m_blob = aKey;
						aBlobNode->Set(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();
						JELLY_ASSERT(req.m_result == RESULT_OK);

						(*aSetCounter)++;
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.m_key = aKey;
						req.m_lock = aLockId;
						req.m_blobSeq = blobSeq;
						req.m_blobNodeIds = { 1 };
						aLockNode->Unlock(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}
				}

				(*aThreadCounter)--;
			}

			struct ValidKeySet
			{
				void
				Add(
					uint32_t	aKey)
				{
					std::lock_guard lock(m_lock);
					if(m_set.find(aKey) == m_set.end())
					{
						m_array.push_back(aKey);
						m_set.insert(aKey);
					}
				}

				bool
				GetRandom(
					uint32_t&	aOut)
				{
					uint32_t i = m_random();

					std::lock_guard lock(m_lock);				
					if(m_array.size() == 0)
						return false;
					
					aOut = m_array[i % (uint32_t)m_array.size()];
					return true;
				}

				size_t
				GetCount()
				{
					std::lock_guard lock(m_lock);
					return m_array.size();
				}

				// Public data
				std::default_random_engine		m_random;

				std::mutex						m_lock;
				std::unordered_set<uint32_t>	m_set;
				std::vector<uint32_t>			m_array;
			};

			void
			_HammerTestThreadWriteBlobs(
				LockNodeType*			aLockNode,
				BlobNodeType*			aBlobNode,
				std::atomic_bool*		aStopEvent,
				uint32_t				aLockId,
				uint32_t				aKeyCount,
				std::atomic_int*		aLockCounter,
				std::atomic_int*		aSetCounter,
				std::atomic_int*		/*aGetCounter*/,
				ValidKeySet*			aValidKeySet,
				std::atomic_int*		aThreadCounter)
			{
				std::default_random_engine generator;
				generator.seed(aLockId);

				std::uniform_int_distribution<uint32_t> keyDistribution(1, aKeyCount);

				while(!*aStopEvent)
				{	
					uint32_t key = keyDistribution(generator);

					uint32_t blobSeq = UINT32_MAX;
					uint32_t blobNodeId = UINT32_MAX;

					// Try to lock
					{
						LockNodeType::Request req;
						req.m_key = key;
						req.m_lock = aLockId;
						aLockNode->Lock(&req);
						while(!req.m_completed.Poll())
							std::this_thread::yield();
						if(req.m_result == RESULT_ALREADY_LOCKED)
							continue;
						JELLY_ASSERT(req.m_result == RESULT_OK);
						blobSeq = req.m_blobSeq;
						if(req.m_blobNodeIds != UINT32_MAX)
						{
							blobNodeId = req.m_blobNodeIds & 0x000000FF;
							JELLY_ASSERT(blobNodeId == 1);
						}

						(*aLockCounter)++;
					}

					// Do a single set
					{
						BlobNodeType::Request req;
						req.m_key = key;
						req.m_seq = ++blobSeq;
						req.m_blob = key;
						aBlobNode->Set(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();						

						if(req.m_result == RESULT_OUTDATED)
							blobSeq = req.m_seq;
						else
							JELLY_ASSERT(req.m_result == RESULT_OK);

						(*aSetCounter)++;
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.m_key = key;
						req.m_lock = aLockId;
						req.m_blobSeq = blobSeq;
						req.m_blobNodeIds = 1;
						aLockNode->Unlock(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();
						JELLY_ASSERT(req.m_result == RESULT_OK);

						aValidKeySet->Add(key);
					}
				}

				(*aThreadCounter)--;
			}

			void
			_HammerTestThreadReadBlobs(
				BlobNodeType*			aBlobNode,
				std::atomic_bool*		aStopEvent,
				uint32_t				/*aKeyCount*/,
				std::atomic_int*		/*aLockCounter*/,
				std::atomic_int*		/*aSetCounter*/,
				std::atomic_int*		aGetCounter,
				ValidKeySet*			aValidKeySet,
				std::atomic_int*		aThreadCounter)
			{
				while(!*aStopEvent)
				{	
					uint32_t key;
					if(!aValidKeySet->GetRandom(key))
					{
						std::this_thread::yield();
						continue;
					}

					// Do a get (no locking needed)
					{
						BlobNodeType::Request req;
						req.m_key = key;
						req.m_seq = 0;
						aBlobNode->Get(&req);
						while (!req.m_completed.Poll())
							std::this_thread::yield();
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == key);

						(*aGetCounter)++;
					}
				}

				(*aThreadCounter)--;
			}

			enum HammerTestMode
			{
				HAMMER_TEST_MODE_GENERAL,
				HAMMER_TEST_MODE_DISK_READ
			};

			void
			_HammerTest(
				DefaultHost*	aHost,
				uint32_t		aKeyCount,
				HammerTestMode	aHammerTestMode)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				// Run the whole thing twice (without cleaning up inbetween), so the second time it will have to restore everything

				for(uint32_t pass = 0; pass < 2; pass++)
				{
					printf("--- PASS %u ---\n", pass);
					uint32_t totalSets = 0;
					uint32_t totalGets = 0;

					// Very stupid thread pool
					struct ThreadPool
					{
						ThreadPool(
							uint32_t			aThreadCount)
						{
							m_stopEvent = false; 

							for(uint32_t i = 0; i < aThreadCount; i++)
							{
								m_threads.push_back(new std::thread([&]()
								{
									while(!m_stopEvent)
									{
										std::function<void()> work;

										{
											std::lock_guard lock(m_workQueueLock);
											if(m_workQueue.size() > 0)
											{
												work = m_workQueue[m_workQueue.size() - 1];
												m_workQueue.pop_back();
											}
										}

										if(work)
											work();

										std::this_thread::yield();
									}
								}));
							}
						}

						~ThreadPool()
						{
							m_stopEvent = true;

							for(std::thread* t : m_threads)
							{
								t->join();
								delete t;
							}
						}

						void
						Post(
							std::function<void()>	aWork)
						{
							std::lock_guard lock(m_workQueueLock);
							m_workQueue.push_back(aWork);
						}

						std::atomic_bool					m_stopEvent;

						std::vector<std::thread*>			m_threads;

						std::mutex							m_workQueueLock;
						std::vector<std::function<void()>>	m_workQueue;
					};
				
					std::vector<std::thread> threads;
					std::atomic_int threadCounter = 0;
					std::atomic_bool stopEvent = false;

					std::chrono::steady_clock::time_point initT0 = std::chrono::steady_clock::now();

					LockNodeType::Config lockNodeConfig;
					lockNodeConfig.m_node.m_walConcurrency = 4;
					LockNodeType lockNode(aHost, 0, lockNodeConfig);

					std::chrono::steady_clock::time_point initT1 = std::chrono::steady_clock::now();

					BlobNodeType::Config blobNodeConfig;
					blobNodeConfig.m_node.m_walConcurrency = 4;

					if (aHammerTestMode == HAMMER_TEST_MODE_DISK_READ)
						blobNodeConfig.m_maxResidentBlobSize = 0;

					BlobNodeType blobNode(aHost, 1, blobNodeConfig);

					std::chrono::steady_clock::time_point initT2 = std::chrono::steady_clock::now();

					double lockNodeInitTime = std::chrono::duration<double>(initT1 - initT0).count();
					double blobNodeInitTime = std::chrono::duration<double>(initT2 - initT1).count();

					printf("lock restore time: %f, blob restore time: %f\n", lockNodeInitTime, blobNodeInitTime);

					std::atomic_int lockCounter = 0;
					std::atomic_int setCounter = 0;
					std::atomic_int getCounter = 0;

					std::unique_ptr<ValidKeySet> validKeySet;

					switch(aHammerTestMode)
					{
					case HAMMER_TEST_MODE_GENERAL:
						{
							uint32_t lockId = 1;
							for (uint32_t i = 0; i < aKeyCount; i++)
							{
								// Two threads fighting over the same key
								threadCounter += 2;

								threads.push_back(std::thread(_HammerTestThreadGeneral, &lockNode, &blobNode, &stopEvent, lockId++, i + 1, &lockCounter, &setCounter, &getCounter, &threadCounter));
								threads.push_back(std::thread(_HammerTestThreadGeneral, &lockNode, &blobNode, &stopEvent, lockId++, i + 1, &lockCounter, &setCounter, &getCounter, &threadCounter));
							}
						}
						break;

					case HAMMER_TEST_MODE_DISK_READ:
						{
							uint32_t lockId = 1;

							validKeySet = std::make_unique<ValidKeySet>();

							// Make a few threads for writing blobs
							for (uint32_t i = 0; i < 10; i++)
							{
								threadCounter++;
								threads.push_back(std::thread(_HammerTestThreadWriteBlobs, &lockNode, &blobNode, &stopEvent, lockId++, aKeyCount, &lockCounter, &setCounter, &getCounter, validKeySet.get(), &threadCounter));
							}

							// And some threads for reading blobs
							for (uint32_t i = 0; i < 100; i++)
							{
								threadCounter++;
								threads.push_back(std::thread(_HammerTestThreadReadBlobs, &blobNode, &stopEvent, aKeyCount, &lockCounter, &setCounter, &getCounter, validKeySet.get(), &threadCounter));
							}
					}
						break;

					default:
						JELLY_ASSERT(false);
					}

					ThreadPool threadPool(lockNodeConfig.m_node.m_walConcurrency + blobNodeConfig.m_node.m_walConcurrency);

					std::chrono::steady_clock::time_point lastInfoPrintTime = std::chrono::steady_clock::now();

					uint32_t seconds = 0;

					while(threadCounter > 0)
					{
						lockNode.ProcessRequests();
						blobNode.ProcessRequests();

						std::atomic_uint32_t waiting = lockNodeConfig.m_node.m_walConcurrency + blobNodeConfig.m_node.m_walConcurrency;

						for (uint32_t i = 0; i < lockNodeConfig.m_node.m_walConcurrency; i++)
							threadPool.Post([i, &lockNode, &waiting]() mutable { lockNode.FlushPendingWAL(i); waiting--; });

						for (uint32_t i = 0; i < blobNodeConfig.m_node.m_walConcurrency; i++)
							threadPool.Post([i, &blobNode, &waiting]() mutable { blobNode.FlushPendingWAL(i); waiting--; });

						while(waiting > 0)
							std::this_thread::yield();

						std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
						double elapsed = std::chrono::duration<double>(currentTime - lastInfoPrintTime).count();
						if(elapsed > 1.0)
						{
							int locks = lockCounter.exchange(0);
							int sets = setCounter.exchange(0);
							int gets = getCounter.exchange(0);
							printf("%d locks, %d sets, %d gets, total resident blob size: %u", locks, sets, gets, (uint32_t)blobNode.GetTotalResidentBlobSize());

							totalSets += (uint32_t)sets;
							totalGets += (uint32_t)gets;

							if(validKeySet)
								printf(", keys: %u\n", (uint32_t)validKeySet->GetCount());
							else
								printf("\n");

							lastInfoPrintTime = currentTime;

							seconds++;

							if (seconds % 5 == 0)
							{
								printf("lock: flush pending store...\n");
								lockNode.FlushPendingStore();
							}
							if (seconds % 6 == 0)
							{
								printf("blob: flush pending store...\n");
								blobNode.FlushPendingStore();
							}
							if (seconds % 7 == 0)
							{
								printf("lock: cleanup WALs...\n");
								lockNode.CleanupWALs();
							}
							if (seconds % 8 == 0)
							{
								printf("blob: cleanup WALs...\n");
								blobNode.CleanupWALs();
							}
							if (seconds % 7 == 0)
							{
								printf("lock: perform compaction...\n");
								_PerformCompaction(&lockNode, aHost);
							}
							if (seconds % 8 == 0)
							{
								printf("blob: perform compaction...\n");
								_PerformCompaction(&blobNode, aHost);
							}

							if(seconds == 60)
							{
								printf("total sets: %u, total gets: %u\n", totalSets, totalGets);
								stopEvent = true;
							}
						}

						std::this_thread::yield();
					}

					JELLY_ASSERT(stopEvent);

					for(std::thread& t : threads)
						t.join();
				}
			}
			
			void
			_TestBlobNode(
				DefaultHost*	aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				{
					BlobNodeType blobNode(aHost, 0);

					// Do a set
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 1;
						req.m_blob = 123;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Do another set with lower sequence number, should fail
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 0;
						req.m_blob = 456;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OUTDATED);
					}

					// Now do a get
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == 123);
						JELLY_ASSERT(req.m_seq == 1);
					}

					_VerifyResidentKeys(&blobNode, { 1 });
				}

				_VerifyBlobNodeWAL<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 1, 123 } });
				_VerifyNoWAL(aHost, 0, 1);

				// Restart node and try more requests

				{
					BlobNodeType blobNode(aHost, 0);
					_VerifyResidentKeys(&blobNode, { 1 });

					// Do another set with lower sequence number again, should still fail
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 0;
						req.m_blob = 456;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OUTDATED);
					}

					// Do a proper set with updated sequence number
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 2;
						req.m_blob = 789;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Verify with a get
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == 789);
						JELLY_ASSERT(req.m_seq == 2);
					}

					// Try getting something with a minimum sequence number that doesn't exist
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 100;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OUTDATED);
					}

					blobNode.FlushPendingStore();
					blobNode.CleanupWALs();

					_VerifyResidentKeys(&blobNode, { 1 });
				}

				_VerifyNoWAL(aHost, 0, 0);
				_VerifyBlobNodeWAL<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 1, { { 1, 2, 789 } });
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 2, 789 } });

				// Do 1 write with another key which we'll try to load later after compaction
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.m_key = 2;
					req.m_seq = 1;
					req.m_blob = 1234;
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_OK);
				}

				// Do 2 writes, each with a flush pending store. This will cause us to have 3
				// store files each with the same blob.
				for(uint32_t i = 0; i < 2; i++)
				{
					BlobNodeType blobNode(aHost, 0);

					// Do another set
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 3 + i;
						req.m_blob = 1000 + i;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					blobNode.FlushPendingStore();

					_VerifyResidentKeys(&blobNode, { 2, 1 });
				}

				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 2, 789 } });
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 1, { { 1, 3, 1000 }, {2, 1, 1234 } });
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 2, { { 1, 4, 1001 } });

				// Perform compaction, removing two of the store files and create a new one
				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0;

					BlobNodeType blobNode(aHost, 0, config);
					_VerifyResidentKeys(&blobNode, { });

					{
						std::unique_ptr<CompactionResultType> compactionResult(blobNode.PerformCompaction(CompactionJob(0, 0, 1)));
						blobNode.ApplyCompactionResult(compactionResult.get());
					}

					_VerifyResidentKeys(&blobNode, { });

					// Do a get
					{
						BlobNodeType::Request req;
						req.m_key = 2;
						req.m_seq = 0;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == 1234);
						JELLY_ASSERT(req.m_seq == 1);
					}

					_VerifyResidentKeys(&blobNode, { });
				}

				_VerifyNoStore(aHost, 0, 0);
				_VerifyNoStore(aHost, 0, 1);
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 2, { { 1, 4, 1001 } });
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 3, { { 1, 3, 1000 }, { 2, 1, 1234 } });
			}

			void
			_TestLockNode(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
		
				{
					LockNodeType lockNode(aHost, 0);					
				
					// Lock
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 123;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blobSeq == UINT32_MAX);
						JELLY_ASSERT(req.m_blobNodeIds == 0xFFFFFFFF);
					}
				}	

				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, 0xFFFFFFFF } });
				
				// Restart node

				{
					LockNodeType lockNode(aHost, 0);

					// Try applying another lock, should fail
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 456;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_ALREADY_LOCKED);
					}

					// Apply existing lock again, should work
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 123;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blobSeq == UINT32_MAX);
						JELLY_ASSERT(req.m_blobNodeIds == 0xFFFFFFFF);
					}

					// Unlock with wrong lock, should fail
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 456;
						lockNode.Unlock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_ALREADY_LOCKED);
					}

					// Unlock key that doesn't exist
					{
						LockNodeType::Request req;
						req.m_key = 2;
						req.m_lock = 123;
						lockNode.Unlock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_DOES_NOT_EXIST);
					}

					// Unlock correctly
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 123;
						req.m_blobSeq = 1;
						req.m_blobNodeIds = 0xFF030201;
						lockNode.Unlock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}
				}

				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, 0xFFFFFFFF } });
				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 1, { { 1, 2, 0, 1, 0xFF030201 } });

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Apply lock, should work now
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 456;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blobSeq == 1);
						JELLY_ASSERT(req.m_blobNodeIds == 0xFF030201);
					}

					// Also apply locks on another two keys in descending order, 
					// so we can check that they'll get sorted for the store
					{
						LockNodeType::Request req;
						req.m_key = 3;
						req.m_lock = 456;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blobSeq == UINT32_MAX);
						JELLY_ASSERT(req.m_blobNodeIds == 0xFFFFFFFF);
					}

					{
						LockNodeType::Request req;
						req.m_key = 2;
						req.m_lock = 456;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blobSeq == UINT32_MAX);
						JELLY_ASSERT(req.m_blobNodeIds == 0xFFFFFFFF);
					}

					lockNode.FlushPendingStore();
				}

				_VerifyNoWAL(aHost, 0, 0);
				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 1, { { 1, 2, 0, 1, 0xFF030201 } });
				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 2,
				{ 
					{ 1, 3, 456, 1, 0xFF030201 },
					{ 3, 1, 456, UINT32_MAX, 0xFFFFFFFF },
					{ 2, 1, 456, UINT32_MAX, 0xFFFFFFFF }
				});
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0,
				{ 
					{ 1, 3, 456, 1, 0xFF030201 },
					{ 2, 1, 456, UINT32_MAX, 0xFFFFFFFF },
					{ 3, 1, 456, UINT32_MAX, 0xFFFFFFFF }
				});
			}

			void
			_TestCancel(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				// Do a little request without canceling it
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.m_key = 1;
					req.m_seq = 1;
					req.m_blob = 101;
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_OK);
				}

				// Do another little request, but cancel it (before processing it)
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.m_key = 2;
					req.m_seq = 1;
					req.m_blob = 101;
					blobNode.Set(&req);
					blobNode.Stop();
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_CANCELED);
				}

				// Cancel after processing, but before flushing WAL
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.m_key = 3;
					req.m_seq = 1;
					req.m_blob = 101;
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.Stop();
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_CANCELED);
				}

				// Cancel after processing and flushing WAL (request should be completed succesfully)
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.m_key = 4;
					req.m_seq = 1;
					req.m_blob = 101;
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					blobNode.Stop();
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_OK);
				}

				// Do request after stopping node, should be canceled immediately
				{
					BlobNodeType blobNode(aHost, 0);

					blobNode.Stop();

					BlobNodeType::Request req;
					req.m_key = 5;
					req.m_seq = 1;
					req.m_blob = 101;
					blobNode.Set(&req);
					JELLY_ASSERT(req.m_completed.Poll());
					JELLY_ASSERT(req.m_result == RESULT_CANCELED);
				}
			}

			void
			_TestLockNodeDelete(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				{
					LockNodeType lockNode(aHost, 0);

					// Lock
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 100;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Try to delete locked key, should fail
					{
						LockNodeType::Request req;
						req.m_key = 1;
						lockNode.Delete(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_ALREADY_LOCKED);
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.m_key = 1;
						req.m_lock = 100;
						req.m_blobNodeIds = 0;
						req.m_blobSeq = 1;
						lockNode.Unlock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Delete
					{
						LockNodeType::Request req;
						req.m_key = 1;
						lockNode.Delete(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}
				}

				// Only thing on disk should be a WAL with 3 entries - lock, unlock, and finally delete (tombstone'd)
				_VerifyLockNodeWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0,
				{ 
					{ 1, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX },
					{ 1, 2, 0, 1, 0, UINT32_MAX },
					{ 1, 3, 0, UINT32_MAX, UINT32_MAX, 0 } // Blod seq/node ids reset, tombstone store id set to 0
				});

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Flush pending store and cleanup WALs - this should result in having 1 store with just a tombstone
					lockNode.FlushPendingStore();

					// Lock something else so we can create three other stores
					for(uint32_t i = 0; i < 3; i++)
					{
						LockNodeType::Request req;
						req.m_key = 2 + i;
						req.m_lock = 100;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						lockNode.FlushPendingStore();
					}
				}

				// Now we should have 4 stores - id 0 with a tombstone, id 1 with the other lock, id 2 with another lock, id 3 with last lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0, { { 1, 3, 0, UINT32_MAX, UINT32_MAX, 0 } }); // key 1 tombstone
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 1, { { 2, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 2 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 3, { { 4, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 4 lock

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Do a compaction
					{
						std::unique_ptr<CompactionResultType> compactionResult(lockNode.PerformCompaction(CompactionJob(0, 0, 1)));
						lockNode.ApplyCompactionResult(compactionResult.get());
					}
				}

				// Now we should have 1 store less
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 3, { { 4, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 4 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 4,
				{ 
					{ 1, 3, 0, UINT32_MAX, UINT32_MAX, 0 }, // key 1 tombstone
					{ 2, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX }, // key 2 lock
				}); 

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Make another store, because we can't compact the latest store
					{
						LockNodeType::Request req;
						req.m_key = 5;
						req.m_lock = 100;
						lockNode.Lock(&req);
						JELLY_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						lockNode.FlushPendingStore();
					}

					// Do a compaction 
					std::unique_ptr<CompactionResultType> compactionResult(lockNode.PerformCompaction(CompactionJob(2, 3, 4)));
					lockNode.ApplyCompactionResult(compactionResult.get());
				}

				// Tombstone should be gone now
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 5, { { 5, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } }); // key 5 lock
				_VerifyLockNodeStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 6,
				{ 
					{ 2, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX }, // key 2 lock
					{ 4, 1, 100, UINT32_MAX, UINT32_MAX, UINT32_MAX } // key 4 lock
				}); 
			}

			void
			_TestBlobNodeDelete(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				// Not gonna test tombstone removal through compaction as it works exactly the same way as for lock nodes

				{
					BlobNodeType blobNode(aHost, 0);

					// Set
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 1;
						req.m_blob = 123;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 });

					// Delete
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 2;
						blobNode.Delete(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 }); // Tombstones are counted as being resident, just 0 size

					// Get - should fail
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_DOES_NOT_EXIST);
					}

					// Set again, should override the delete
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = 3;
						req.m_blob = 1234;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 });

					// Get - now it should work
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == 1234);
					}
				}
			}

			void
			_TestBlobNodeMemoryLimit(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobCount = 5; // Using count limit as we can't really predict compressed blob sizes 

					BlobNodeType blobNode(aHost, 0, config);
					
					// Do a bunch of sets with different keys
					for(uint32_t key = 1; key <= 5; key++)
					{
						BlobNodeType::Request req;
						req.m_key = key;
						req.m_seq = 1;
						req.m_blob = 100 + key;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Verify all of them are now resident
					_VerifyResidentKeys(&blobNode, { 1, 2, 3, 4, 5} );			

					// Do another set (on a new key), this shouldn't expell anything as everything is still in the WAL
					{
						BlobNodeType::Request req;
						req.m_key = 6;
						req.m_seq = 1;
						req.m_blob = 100 + 6;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 1, 2, 3, 4, 5, 6 });

					// Now flush pending store, clearing WAL
					blobNode.FlushPendingStore();

					// Write again now that WAL is cleared
					{
						BlobNodeType::Request req;
						req.m_key = 7;
						req.m_seq = 1;
						req.m_blob = 100 + 7;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// Now oldest should be expelled down to 5 blobs
					_VerifyResidentKeys(&blobNode, { 3, 4, 5, 6, 7 });

					// Now try to update the oldest item, which should move it to the front
					{
						BlobNodeType::Request req;
						req.m_key = 3;
						req.m_seq = 2; // Incremented sequence number
						req.m_blob = 103;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 4, 5, 6, 7, 3 });

					// Try to get one of the expelled keys
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_blob == 101);
						JELLY_ASSERT(req.m_seq == 1);
					}

					_VerifyResidentKeys(&blobNode, { 5, 6, 7, 3, 1 });

					// Try to set an expelled key
					{
						BlobNodeType::Request req;
						req.m_key = 2;
						req.m_seq = 2; // Incremented sequence number
						req.m_blob = 102;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 6, 7, 3, 1, 2 });
				}

				// Do a different test now, with memory limit being hit from the get-go

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0; // No space for anything

					BlobNodeType blobNode(aHost, 1, config);
					
					// Set a few keys
					for(uint32_t i = 1; i <= 3; i++)
					{
						BlobNodeType::Request req;
						req.m_key = i;
						req.m_seq = 1; 
						req.m_blob = 100 + i;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
					}

					// All keys should be resident because they're still in the WAL
					_VerifyResidentKeys(&blobNode, { 1, 2, 3 });

					blobNode.FlushPendingStore();

					// After flushing store there should be nothing in WAL, hence nothing resident as well
					_VerifyResidentKeys(&blobNode, { });
				}

				// Do a restart

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0; // Still no space for anything

					BlobNodeType blobNode(aHost, 1, config);

					// We should no longer have any resident keys as memory limit caused nothing to be loaded from stores and there is no
					// fresh updates in WALs
					_VerifyResidentKeys(&blobNode, { });
				}

				// Again with the memory limit being immediate, but make two copies of the same blob across two stores

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0; 

					BlobNodeType blobNode(aHost, 2, config);

					// Store the same key twice
					for (uint32_t i = 1; i <= 2; i++)
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						req.m_seq = i;
						req.m_blob = 100 + i;
						blobNode.Set(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK); 

						blobNode.FlushPendingStore();
					}
				}

				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 2, 0, { { 1, 1, 101 } });
				_VerifyBlobNodeStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 2, 1, { { 1, 2, 102 } });

				// Restart

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0;

					BlobNodeType blobNode(aHost, 2, config);

					// Check nothing is resident
					_VerifyResidentKeys(&blobNode, { });

					// Read the blob and see that it's the latest
					{
						BlobNodeType::Request req;
						req.m_key = 1;
						blobNode.Get(&req);
						JELLY_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ASSERT(req.m_completed.Poll());
						JELLY_ASSERT(req.m_result == RESULT_OK);
						JELLY_ASSERT(req.m_seq == 2);
						JELLY_ASSERT(req.m_blob == 102);
					}
				}
			}

		}

		namespace NodeTest
		{

			void		
			Run(
				const char*		aWorkingDirectory,
				const Config*	aConfig)
			{
				std::filesystem::create_directories(aWorkingDirectory);

				#if defined(JELLY_ZSTD)
					TestDefaultHost host(aWorkingDirectory, DefaultHost::COMPRESSION_MODE_ZSTD);
				#else 
					TestDefaultHost host(aWorkingDirectory, DefaultHost::COMPRESSION_MODE_NONE);
				#endif

				// Test basic operation of BlobNode
				_TestBlobNode(&host);

				// Test basic operation of LockNode
				_TestLockNode(&host);

				// Test delete operation of LockNode
				_TestLockNodeDelete(&host);

				// Test delete operation of BlobNode
				_TestBlobNodeDelete(&host);

				// Test canceling requests (exactly same code for lock and blob nodes)
				_TestCancel(&host); 

				// Test blob node with memory limit turned on
				_TestBlobNodeMemoryLimit(&host);

				if(aConfig->m_hammerTest)
				{
					// Run a general "hammer test" that will test LockNode and BlobNode in a multithreaded environment
					_HammerTest(&host, 100, HAMMER_TEST_MODE_GENERAL);

					// Another "hammer test" which will have memory caching turned off and thus will trigger a lot of 
					// reads from disk
					_HammerTest(&host, 1000000, HAMMER_TEST_MODE_DISK_READ);
				}
			}

		}

	}

}