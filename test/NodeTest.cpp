// Big pile of node testing mess. Not making any allusions about this providing full covering of everything,
// as it's more of a "shotgun" approach to testing. It covers a lot of stuff, but definetely not all.

#include <optional>
#include <random>
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

			typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
			typedef BlobNodeItem<UIntKey<uint32_t>, MetaData::Dummy> BlobNodeItemType;

			typedef MetaData::LockStaticSingleBlob<4> LockMetaDataType;
			typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType> LockNodeType;
			typedef LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>, LockMetaDataType> LockNodeItemType;

			typedef CompactionResult<UIntKey<uint32_t>> CompactionResultType;

			struct ExpectedBlobNodeItemData
			{
				bool
				CompareItem(
					const Compression::IProvider*					aCompression,
					const BlobNodeItemType&							aItem) const
				{
					uint32_t blobValue;
					
					if(aCompression != NULL)
					{
						std::unique_ptr<IBuffer> uncompressed(aCompression->DecompressBuffer(aItem.GetBlob()));
						blobValue = UInt32Blob::GetValue(uncompressed.get());
					}
					else
					{
						blobValue = UInt32Blob::GetValue(aItem.GetBlob());
					}
					
					return m_key == aItem.GetKey() && m_seq == aItem.GetSeq() && blobValue == m_blob;
				}

				// Public data
				uint32_t				m_key;
				uint32_t				m_seq;
				uint32_t				m_blob;
			};

			struct ExpectedLockNodeItemData
			{
				bool
				CompareItem(
					const Compression::IProvider*					/*aCompression*/,
					const LockNodeItemType&							aItem) const
				{
					if(m_key == aItem.GetKey() && m_lock == aItem.GetLock() && m_seq == aItem.GetSeq() && m_tombstoneStoreId == aItem.GetTombstoneStoreId() &&
						m_blobSeq == aItem.GetMeta().m_blobSeq && m_blobNodeIds.size() == (size_t)aItem.GetMeta().m_blobNodeIdCount)
					{
						for(size_t i = 0; i < m_blobNodeIds.size(); i++)
						{
							if(m_blobNodeIds[i] != aItem.GetMeta().m_blobNodeIds[i])
								return false;
						}
						
						return true;
					}

					return false;
				}

				// Public data
				uint32_t				m_key;
				uint32_t				m_seq;
				uint32_t				m_lock;
				uint32_t				m_blobSeq;
				std::vector<uint32_t>	m_blobNodeIds;
				uint32_t				m_tombstoneStoreId;
			};

			template <typename _NodeType>
			void
			_PerformCompaction(
				_NodeType*											aNode,
				IHost*												aHost)
			{
				DefaultConfigSource configSource;
				configSource.Set(jelly::Config::ID_COMPACTION_SIZE_MEMORY, "1");
				configSource.Set(jelly::Config::ID_COMPACTION_SIZE_TREND_MEMORY, "1");
				configSource.Set(jelly::Config::ID_COMPACTION_STRATEGY_UPDATE_INTERVAL_MS, "0");
				configSource.Set(jelly::Config::ID_STCS_MIN_BUCKET_SIZE, "2");

				ConfigProxy config(&configSource);

				CompactionAdvisor compactionAdvisor(aNode->GetNodeId(), aHost, &config, CompactionAdvisor::STRATEGY_SIZE_TIERED);
				compactionAdvisor.Update();

				CompactionJob compactionJob = compactionAdvisor.GetNextSuggestion();
				if(compactionJob.IsSet())
				{					
					std::unique_ptr<CompactionResultType> compactionResult(aNode->PerformCompaction(compactionJob));
					aNode->ApplyCompactionResult(compactionResult.get());
				}
			}

			template <typename _ItemType, typename _ExpectedItemType>
			void
			_VerifyFileStreamReader(
				Compression::IProvider*								aCompression,
				IFileStreamReader*									aFileStreamReader,
				const std::vector<_ExpectedItemType>&				aExpected)
			{
				JELLY_UNUSED(aCompression);

				std::unique_ptr<IFileStreamReader> f(aFileStreamReader);
				JELLY_ALWAYS_ASSERT(f);

				for (const _ExpectedItemType& expected : aExpected)
				{
					JELLY_UNUSED(expected);
					JELLY_ALWAYS_ASSERT(!f->IsEnd());
					_ItemType item;
					JELLY_ALWAYS_ASSERT(item.Read(f.get(), NULL));
					JELLY_ALWAYS_ASSERT(expected.CompareItem(aCompression, item));
				}

				JELLY_ALWAYS_ASSERT(f->IsEnd());
			}

			void
			_VerifyLockNodeWAL(
				DefaultHost*											aHost,
				uint32_t												aNodeId,
				uint32_t												aId,
				const std::vector<ExpectedLockNodeItemData>&			aExpected)
			{
				_VerifyFileStreamReader<LockNodeItemType, ExpectedLockNodeItemData>(NULL, aHost->ReadWALStream(aNodeId, aId, true, NULL), aExpected);
			}

			void
			_VerifyBlobNodeWAL(
				DefaultHost*											aHost,
				uint32_t												aNodeId,
				uint32_t												aId,
				const std::vector<ExpectedBlobNodeItemData>&			aExpected)
			{
				_VerifyFileStreamReader<BlobNodeItemType, ExpectedBlobNodeItemData>(aHost->GetCompressionProvider(), aHost->ReadWALStream(aNodeId, aId, false, NULL), aExpected);
			}

			void
			_VerifyLockNodeStore(
				DefaultHost*											aHost,
				uint32_t												aNodeId,
				uint32_t												aId,
				const std::vector<ExpectedLockNodeItemData>&			aExpected)
			{
				_VerifyFileStreamReader<LockNodeItemType, ExpectedLockNodeItemData>(NULL, aHost->ReadStoreStream(aNodeId, aId, NULL), aExpected);
			}

			void
			_VerifyBlobNodeStore(
				DefaultHost*											aHost,
				uint32_t												aNodeId,
				uint32_t												aId,
				const std::vector<ExpectedBlobNodeItemData>&			aExpected)
			{
				_VerifyFileStreamReader<BlobNodeItemType, ExpectedBlobNodeItemData>(aHost->GetCompressionProvider(), aHost->ReadStoreStream(aNodeId, aId, NULL), aExpected);
			}

			void
			_VerifyNoWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId)
			{
				std::unique_ptr<IFileStreamReader> f(aHost->ReadWALStream(aNodeId, aId, false, NULL));
				JELLY_ALWAYS_ASSERT(!f);
			}

			void
			_VerifyNoStore(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId)
			{
				std::unique_ptr<IFileStreamReader> f(aHost->ReadStoreStream(aNodeId, aId, NULL));
				JELLY_ALWAYS_ASSERT(!f);
			}

			template <typename _NodeType>
			void
			_VerifyResidentKeys(
				_NodeType*													aNode,
				const std::vector<UIntKey<uint32_t>>&						aExpected)
			{
				std::vector<UIntKey<uint32_t>> keys;
				aNode->GetResidentKeys(keys);
				JELLY_UNUSED(aExpected);
				JELLY_ALWAYS_ASSERT(keys == aExpected);
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
						req.SetKey(aKey);
						req.SetLock(aLockId);
						aLockNode->Lock(&req);
						while(!req.IsCompleted())
							std::this_thread::yield();
						if(req.GetResult() == REQUEST_RESULT_ALREADY_LOCKED)
							continue;
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						blobSeq = req.GetMeta().m_blobSeq;
						if(req.GetMeta().m_blobNodeIdCount > 0)
						{	
							JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 1);
							blobNodeId = req.GetMeta().m_blobNodeIds[0];
						}

						(*aLockCounter)++;
					}

					// If we have a blob, get it
					if(blobSeq != UINT32_MAX)
					{
						JELLY_ALWAYS_ASSERT(blobNodeId == 1);

						BlobNodeType::Request req;
						req.SetKey(aKey);
						req.SetSeq(blobSeq);
						aBlobNode->Get(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == aKey);

						blobSeq = req.GetSeq();

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
						req.SetKey(aKey);
						req.SetSeq(++blobSeq);
						req.SetBlob(new UInt32Blob(aKey));
						aBlobNode->Set(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);

						(*aSetCounter)++;
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.SetKey(aKey);
						req.SetLock(aLockId);
						req.SetMeta(LockMetaDataType(blobSeq, { 1 }));
						aLockNode->Unlock(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
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
						req.SetKey(key);
						req.SetLock(aLockId);
						aLockNode->Lock(&req);
						while(!req.IsCompleted())
							std::this_thread::yield();
						if(req.GetResult() == REQUEST_RESULT_ALREADY_LOCKED)
							continue;
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						blobSeq = req.GetMeta().m_blobSeq;
						if(req.GetMeta().m_blobNodeIdCount > 0)
						{
							JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 1);
							blobNodeId = req.GetMeta().m_blobNodeIds[0];
							JELLY_ALWAYS_ASSERT(blobNodeId == 1);
						}

						(*aLockCounter)++;
					}

					// Do a single set
					{
						BlobNodeType::Request req;
						req.SetKey(key);
						req.SetSeq(++blobSeq);
						req.SetBlob(new UInt32Blob(key));
						aBlobNode->Set(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();						

						if(req.GetResult() == REQUEST_RESULT_OUTDATED)
							blobSeq = req.GetSeq();
						else
							JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);

						(*aSetCounter)++;
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.SetKey(key);
						req.SetLock(aLockId);
						req.SetMeta(LockMetaDataType(blobSeq, { 1 }));
						aLockNode->Unlock(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);

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
						req.SetKey(key);
						req.SetSeq(0);
						aBlobNode->Get(&req);
						while (!req.IsCompleted())
							std::this_thread::yield();
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == key);

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
				TestDefaultHost*	aHost,
				uint32_t			aKeyCount,
				HammerTestMode		aHammerTestMode)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				aHost->GetDefaultConfigSource()->Clear();
				
				uint32_t walConcurrency = 4;
				aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_WAL_CONCURRENCY, "4");

				if (aHammerTestMode == HAMMER_TEST_MODE_DISK_READ)
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_SIZE, "0");

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

					LockNodeType lockNode(aHost, 0);

					std::chrono::steady_clock::time_point initT1 = std::chrono::steady_clock::now();

					BlobNodeType blobNode(aHost, 1);

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
						JELLY_ALWAYS_ASSERT(false);
					}

					ThreadPool threadPool(walConcurrency * 2);

					std::chrono::steady_clock::time_point lastInfoPrintTime = std::chrono::steady_clock::now();

					uint32_t seconds = 0;

					while(threadCounter > 0)
					{
						lockNode.ProcessRequests();
						blobNode.ProcessRequests();

						std::atomic_uint32_t waiting = walConcurrency * 2;

						for (uint32_t i = 0; i < walConcurrency; i++)
							threadPool.Post([i, &lockNode, &waiting]() mutable { lockNode.FlushPendingWAL(i); waiting--; });

						for (uint32_t i = 0; i < walConcurrency; i++)
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

					JELLY_ALWAYS_ASSERT(stopEvent);

					for(std::thread& t : threads)
						t.join();
				}
			}
			
			void
			_TestBlobNode(
				TestDefaultHost*	aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				{
					BlobNodeType blobNode(aHost, 0);

					// Do a set
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(1);
						req.SetBlob(new UInt32Blob(123));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Do another set with lower sequence number, should fail
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(0);
						req.SetBlob(new UInt32Blob(456));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OUTDATED);
					}

					// Now do a get
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 123);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 1);
					}

					_VerifyResidentKeys(&blobNode, { 1 });
				}

				_VerifyBlobNodeWAL(aHost, 0, 0, { { 1, 1, 123 } });
				_VerifyNoWAL(aHost, 0, 1);

				// Restart node and try more requests

				{
					BlobNodeType blobNode(aHost, 0);
					_VerifyResidentKeys(&blobNode, { 1 });

					// Do another set with lower sequence number again, should still fail
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(0);
						req.SetBlob(new UInt32Blob(456));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OUTDATED);
					}

					// Do a proper set with updated sequence number
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(2);
						req.SetBlob(new UInt32Blob(789));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Verify with a get
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 789);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 2);
					}

					// Try getting something with a minimum sequence number that doesn't exist
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(100);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OUTDATED);
					}

					blobNode.FlushPendingStore();
					blobNode.CleanupWALs();

					_VerifyResidentKeys(&blobNode, { 1 });
				}

				_VerifyNoWAL(aHost, 0, 0);
				_VerifyBlobNodeWAL(aHost, 0, 1, { { 1, 2, 789 } });
				_VerifyBlobNodeStore(aHost, 0, 0, { { 1, 2, 789 } });

				// Do 1 write with another key which we'll try to load later after compaction
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.SetKey(2);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(1234));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
				}

				// Do 2 writes, each with a flush pending store. This will cause us to have 3
				// store files each with the same blob.
				for(uint32_t i = 0; i < 2; i++)
				{
					BlobNodeType blobNode(aHost, 0);

					// Do another set
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(3 + i);
						req.SetBlob(new UInt32Blob(1000 + i));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					blobNode.FlushPendingStore();

					_VerifyResidentKeys(&blobNode, { 2, 1 });
				}

				_VerifyBlobNodeStore(aHost, 0, 0, { { 1, 2, 789 } });
				_VerifyBlobNodeStore(aHost, 0, 1, { { 1, 3, 1000 }, {2, 1, 1234 } });
				_VerifyBlobNodeStore(aHost, 0, 2, { { 1, 4, 1001 } });

				// Perform compaction, removing two of the store files and create a new one
				{
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_SIZE, "0");
					BlobNodeType blobNode(aHost, 0);
					_VerifyResidentKeys(&blobNode, { });

					{
						std::unique_ptr<CompactionResultType> compactionResult(blobNode.PerformCompaction(CompactionJob(0, 0, 1)));
						blobNode.ApplyCompactionResult(compactionResult.get());
					}

					_VerifyResidentKeys(&blobNode, { });

					// Do a get
					{
						BlobNodeType::Request req;
						req.SetKey(2);
						req.SetSeq(0);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1234);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 1);
					}

					_VerifyResidentKeys(&blobNode, { });
				}

				_VerifyNoStore(aHost, 0, 0);
				_VerifyNoStore(aHost, 0, 1);
				_VerifyBlobNodeStore(aHost, 0, 2, { { 1, 4, 1001 } });
				_VerifyBlobNodeStore(aHost, 0, 3, { { 1, 3, 1000 }, { 2, 1, 1234 } });

				// Make a bunch of stores for major compaction
				{
					aHost->GetDefaultConfigSource()->Clear();
					BlobNodeType blobNode(aHost, 0);

					for(uint32_t i = 0; i < 5; i++)
					{
						BlobNodeType::Request req;
						req.SetKey(1000 + i);
						req.SetSeq(0);
						req.SetBlob(new UInt32Blob(10000 + i));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						blobNode.FlushPendingStore();
					}
				}

				_VerifyBlobNodeStore(aHost, 0, 2, { { 1, 4, 1001 } });
				_VerifyBlobNodeStore(aHost, 0, 3, { { 1, 3, 1000 }, { 2, 1, 1234 } });
				_VerifyBlobNodeStore(aHost, 0, 4, { { 1000, 0, 10000 } });
				_VerifyBlobNodeStore(aHost, 0, 5, { { 1001, 0, 10001 } });
				_VerifyBlobNodeStore(aHost, 0, 6, { { 1002, 0, 10002 } });
				_VerifyBlobNodeStore(aHost, 0, 7, { { 1003, 0, 10003 } });
				_VerifyBlobNodeStore(aHost, 0, 8, { { 1004, 0, 10004 } });

				// Perform major compaction - this should turn all stores (except latest) into a single new store
				{
					BlobNodeType blobNode(aHost, 0);

					{
						std::unique_ptr<CompactionResultType> compactionResult(blobNode.PerformMajorCompaction());
						blobNode.ApplyCompactionResult(compactionResult.get());
					}
				}

				_VerifyBlobNodeStore(aHost, 0, 8, { { 1004, 0, 10004 } });
				_VerifyBlobNodeStore(aHost, 0, 9, 
				{ 
					{ 1, 4, 1001 },
					{ 2, 1,	1234},
					{ 1000, 0, 10000 },
					{ 1001, 0, 10001 },
					{ 1002, 0, 10002 },
					{ 1003, 0, 10003 }
				});
			}

			void
			_TestLockNode(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				{
					LockNodeType lockNode(aHost, 0);					
				
					// Lock
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(123);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobSeq == UINT32_MAX);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 0);
					}
				}	

				_VerifyLockNodeWAL(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, { }, UINT32_MAX } } );
				
				// Restart node

				{
					LockNodeType lockNode(aHost, 0);

					// Try applying another lock, should fail
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(456);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_ALREADY_LOCKED);
					}

					// Apply existing lock again, should work
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(123);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobSeq == UINT32_MAX);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 0);
					}

					// Unlock with wrong lock, should fail
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(456);
						lockNode.Unlock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_ALREADY_LOCKED);
					}

					// Unlock key that doesn't exist
					{
						LockNodeType::Request req;
						req.SetKey(2);
						req.SetLock(123);
						lockNode.Unlock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_DOES_NOT_EXIST);
					}

					// Unlock correctly
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(123);
						req.SetMeta(LockMetaDataType(1, { 1, 2, 3 }));
						lockNode.Unlock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}
				}

				_VerifyLockNodeWAL(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, { }, UINT32_MAX } });
				_VerifyLockNodeWAL(aHost, 0, 1, { { 1, 2, 0, 1, { 1, 2, 3 }, UINT32_MAX } });

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Apply lock, should work now
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(456);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobSeq == 1);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 3);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIds[0] == 1);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIds[1] == 2);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIds[2] == 3);
					}

					// Also apply locks on another two keys in descending order, 
					// so we can check that they'll get sorted for the store
					{
						LockNodeType::Request req;
						req.SetKey(3);
						req.SetLock(456);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobSeq == UINT32_MAX);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 0);
					}

					{
						LockNodeType::Request req;
						req.SetKey(2);
						req.SetLock(456);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobSeq == UINT32_MAX);
						JELLY_ALWAYS_ASSERT(req.GetMeta().m_blobNodeIdCount == 0);
					}

					lockNode.FlushPendingStore();
				}

				_VerifyNoWAL(aHost, 0, 0);
				_VerifyLockNodeWAL(aHost, 0, 1, { { 1, 2, 0, 1, { 1, 2, 3 }, UINT32_MAX } });
				_VerifyLockNodeWAL(aHost, 0, 2,
				{ 
					{ 1, 3, 456, 1, { 1, 2, 3 }, UINT32_MAX },
					{ 3, 1, 456, UINT32_MAX, { }, UINT32_MAX },
					{ 2, 1, 456, UINT32_MAX, { }, UINT32_MAX }
				});
				_VerifyLockNodeStore(aHost, 0, 0,
				{ 
					{ 1, 3, 456, 1, { 1, 2, 3 }, UINT32_MAX },
					{ 2, 1, 456, UINT32_MAX, { }, UINT32_MAX },
					{ 3, 1, 456, UINT32_MAX, { }, UINT32_MAX }
				});
			}

			void
			_TestCancel(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				// Do a little request without canceling it
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.SetKey(1);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(101));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
				}

				// Do another little request, but cancel it (before processing it)
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.SetKey(2);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(101));
					blobNode.Set(&req);
					blobNode.Stop();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_CANCELED);
				}

				// Cancel after processing, but before flushing WAL
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.SetKey(3);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(101));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.Stop();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_CANCELED);
				}

				// Cancel after processing and flushing WAL (request should be completed succesfully)
				{
					BlobNodeType blobNode(aHost, 0);

					BlobNodeType::Request req;
					req.SetKey(4);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(101));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL(0);
					blobNode.Stop();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
				}

				// Do request after stopping node, should be canceled immediately
				{
					BlobNodeType blobNode(aHost, 0);

					blobNode.Stop();

					BlobNodeType::Request req;
					req.SetKey(5);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(101));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_CANCELED);
				}
			}

			void
			_TestLockNodeDelete(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				{
					LockNodeType lockNode(aHost, 0);

					// Lock
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(100);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Try to delete locked key, should fail
					{
						LockNodeType::Request req;
						req.SetKey(1);
						lockNode.Delete(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_ALREADY_LOCKED);
					}

					// Unlock
					{
						LockNodeType::Request req;
						req.SetKey(1);
						req.SetLock(100);
						req.SetMeta(LockMetaDataType(1, { 0 }));
						lockNode.Unlock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Delete
					{
						LockNodeType::Request req;
						req.SetKey(1);
						lockNode.Delete(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}
				}

				// Only thing on disk should be a WAL with 3 entries - lock, unlock, and finally delete (tombstone'd)
				_VerifyLockNodeWAL(aHost, 0, 0,
				{ 
					{ 1, 1, 100, UINT32_MAX, { }, UINT32_MAX },
					{ 1, 2, 0, 1, { 0 }, UINT32_MAX },
					{ 1, 3, 0, UINT32_MAX, { }, 0 } // Blod seq/node ids reset, tombstone store id set to 0
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
						req.SetKey(2 + i);
						req.SetLock(100);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						lockNode.FlushPendingStore();
					}
				}

				// Now we should have 4 stores - id 0 with a tombstone, id 1 with the other lock, id 2 with another lock, id 3 with last lock
				_VerifyLockNodeStore(aHost, 0, 0, { { 1, 3, 0, UINT32_MAX, { }, 0 } }); // key 1 tombstone
				_VerifyLockNodeStore(aHost, 0, 1, { { 2, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 2 lock
				_VerifyLockNodeStore(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore(aHost, 0, 3, { { 4, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 4 lock

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
				_VerifyLockNodeStore(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore(aHost, 0, 3, { { 4, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 4 lock
				_VerifyLockNodeStore(aHost, 0, 4,
				{ 
					{ 1, 3, 0, UINT32_MAX, { }, 0 }, // key 1 tombstone
					{ 2, 1, 100, UINT32_MAX, { }, UINT32_MAX }, // key 2 lock
				}); 

				// Restart

				{
					LockNodeType lockNode(aHost, 0);

					// Make another store, because we can't compact the latest store
					{
						LockNodeType::Request req;
						req.SetKey(5);
						req.SetLock(100);
						lockNode.Lock(&req);
						JELLY_ALWAYS_ASSERT(lockNode.ProcessRequests() == 1);
						lockNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						lockNode.FlushPendingStore();
					}

					// Do a compaction 
					std::unique_ptr<CompactionResultType> compactionResult(lockNode.PerformCompaction(CompactionJob(2, 3, 4)));
					lockNode.ApplyCompactionResult(compactionResult.get());
				}

				// Tombstone should be gone now
				_VerifyLockNodeStore(aHost, 0, 2, { { 3, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 3 lock
				_VerifyLockNodeStore(aHost, 0, 5, { { 5, 1, 100, UINT32_MAX, { }, UINT32_MAX } }); // key 5 lock
				_VerifyLockNodeStore(aHost, 0, 6,
				{ 
					{ 2, 1, 100, UINT32_MAX, { }, UINT32_MAX }, // key 2 lock
					{ 4, 1, 100, UINT32_MAX, { }, UINT32_MAX } // key 4 lock
				}); 
			}

			void
			_TestBlobNodeDelete(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				// Not gonna test tombstone removal through compaction as it works exactly the same way as for lock nodes

				{
					BlobNodeType blobNode(aHost, 0);

					// Set
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(1);
						req.SetBlob(new UInt32Blob(123));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 });

					// Delete
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(2);
						blobNode.Delete(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 }); // Tombstones are counted as being resident, just 0 size

					// Get - should fail
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_DOES_NOT_EXIST);
					}

					// Set again, should override the delete
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(3);
						req.SetBlob(new UInt32Blob(1234));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys<BlobNodeType>(&blobNode, { 1 });

					// Get - now it should work
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1234);
					}
				}
			}

			void
			_TestBlobNodeMemoryLimit(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				{
					// Using count limit as we can't really predict compressed blob sizes 
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_COUNT, "5");

					BlobNodeType blobNode(aHost, 0);
					
					// Do a bunch of sets with different keys
					for(uint32_t key = 1; key <= 5; key++)
					{
						BlobNodeType::Request req;
						req.SetKey(key);
						req.SetSeq(1);
						req.SetBlob(new UInt32Blob(100 + key));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Verify all of them are now resident
					_VerifyResidentKeys(&blobNode, { 1, 2, 3, 4, 5} );			

					// Do another set (on a new key), this shouldn't expell anything as everything is still in the WAL
					{
						BlobNodeType::Request req;
						req.SetKey(6);
						req.SetSeq(1);
						req.SetBlob(new UInt32Blob(100 + 6));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 1, 2, 3, 4, 5, 6 });

					// Now flush pending store, clearing WAL
					blobNode.FlushPendingStore();

					// Write again now that WAL is cleared
					{
						BlobNodeType::Request req;
						req.SetKey(7);
						req.SetSeq(1);
						req.SetBlob(new UInt32Blob(100 + 7));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Now oldest should be expelled down to 5 blobs
					_VerifyResidentKeys(&blobNode, { 3, 4, 5, 6, 7 });

					// Now try to update the oldest item, which should move it to the front
					{
						BlobNodeType::Request req;
						req.SetKey(3);
						req.SetSeq(2); // Incremented sequence number
						req.SetBlob(new UInt32Blob(103));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 4, 5, 6, 7, 3 });

					// Try to get one of the expelled keys
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 101);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 1);
					}

					_VerifyResidentKeys(&blobNode, { 5, 6, 7, 3, 1 });

					// Try to set an expelled key
					{
						BlobNodeType::Request req;
						req.SetKey(2);
						req.SetSeq(2); // Incremented sequence number
						req.SetBlob(new UInt32Blob(102));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					_VerifyResidentKeys(&blobNode, { 6, 7, 3, 1, 2 });
				}

				// Do a different test now, with memory limit being hit from the get-go

				{
					// No space for anything
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_COUNT, "0");

					BlobNodeType blobNode(aHost, 1);
					
					// Set a few keys
					for(uint32_t i = 1; i <= 3; i++)
					{
						BlobNodeType::Request req;
						req.SetKey(i);
						req.SetSeq(1); 
						req.SetBlob(new UInt32Blob(100 + i));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// All keys should be resident because they're still in the WAL
					_VerifyResidentKeys(&blobNode, { 1, 2, 3 });

					blobNode.FlushPendingStore();

					// After flushing store there should be nothing in WAL, hence nothing resident as well
					_VerifyResidentKeys(&blobNode, { });
				}

				// Do a restart

				{
					// Still no space for anything
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_COUNT, "0");

					BlobNodeType blobNode(aHost, 1);

					// We should no longer have any resident keys as memory limit caused nothing to be loaded from stores and there is no
					// fresh updates in WALs
					_VerifyResidentKeys(&blobNode, { });
				}

				// Again with the memory limit being immediate, but make two copies of the same blob across two stores

				{
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_SIZE, "0");

					BlobNodeType blobNode(aHost, 2);

					// Store the same key twice
					for (uint32_t i = 1; i <= 2; i++)
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						req.SetSeq(i);
						req.SetBlob(new UInt32Blob(100 + i));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK); 

						blobNode.FlushPendingStore();
					}
				}

				_VerifyBlobNodeStore(aHost, 2, 0, { { 1, 1, 101 } });
				_VerifyBlobNodeStore(aHost, 2, 1, { { 1, 2, 102 } });

				// Restart

				{
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_SIZE, "0");

					BlobNodeType blobNode(aHost, 2);

					// Check nothing is resident
					_VerifyResidentKeys(&blobNode, { });

					// Read the blob and see that it's the latest
					{
						BlobNodeType::Request req;
						req.SetKey(1);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 2);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 102);
					}
				}
			}

			void
			_TestBlobNodeRepeatedSets(
				TestDefaultHost*		aHost,
				bool					aMemoryLimit)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				if(aMemoryLimit)
					aHost->GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_COUNT, "0");

				{
					BlobNodeType blobNode(aHost, 0);
					
					// Do a bunch of sets on the same key with increasing sequence numbers
					for(uint32_t seq = 1; seq <= 5; seq++)
					{
						BlobNodeType::Request req;
						req.SetKey(123);
						req.SetSeq(seq);
						req.SetBlob(new UInt32Blob(100 + seq));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						blobNode.FlushPendingWAL(0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}
				}

				_VerifyBlobNodeWAL(aHost, 0, 0, { { 123, 1, 101 }, { 123, 2, 102 }, { 123, 3, 103 }, { 123, 4, 104 }, { 123, 5, 105 } });

				// Restart

				{
					BlobNodeType blobNode(aHost, 0);

					// Do a get, we should have the latest version
					{
						BlobNodeType::Request req;
						req.SetKey(123);
						req.SetSeq(5);
						blobNode.Get(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
						JELLY_ALWAYS_ASSERT(req.GetSeq() == 5);
						JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 105);
					}
				}
			}

			void
			_TestBlobNodeLockVerification(
				TestDefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);
				aHost->GetDefaultConfigSource()->Clear();

				{
					BlobNodeType blobNode(aHost, 0);

					// Do a set
					{					
						BlobNodeType::Request req;
						req.SetKey(123);
						req.SetSeq(1);
						req.SetLockSeq(1);
						req.SetBlob(new UInt32Blob(100));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 1);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}

					// Do a set with lower lock sequence number - it should fail
					{
						BlobNodeType::Request req;
						req.SetKey(123);
						req.SetSeq(2);
						req.SetLockSeq(0); // Lower than 1
						req.SetBlob(new UInt32Blob(101));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 0);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_NOT_LOCKED);
					}

					// Do a set with higher lock sequence number - it should work
					{
						BlobNodeType::Request req;
						req.SetKey(123);
						req.SetSeq(3);
						req.SetLockSeq(2); // Higher than 1
						req.SetBlob(new UInt32Blob(102));
						blobNode.Set(&req);
						JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
						JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 1);
						JELLY_ALWAYS_ASSERT(req.IsCompleted());
						JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					}
				}

				_VerifyBlobNodeWAL(aHost, 0, 0, { { 123, 1, 100 }, { 123, 3, 102 } });
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

				TestDefaultHost host(aWorkingDirectory);

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

				// Test blob node restart after setting the same item repeatedly (without flush pending store)
				_TestBlobNodeRepeatedSets(&host, true); // With memory limit
				_TestBlobNodeRepeatedSets(&host, false); // Without memory limit

				// Test blob node lock sequence numbering
				_TestBlobNodeLockVerification(&host);

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