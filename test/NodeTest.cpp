// Big pile of node testing mess. Not making any allusions about this providing full covering of everything,
// as it's more of a "shotgun" approach to testing. It covers a lot of stuff, but definetely not all.

#include <chrono>
#include <filesystem>
#include <optional>
#include <random>
#include <thread>
#include <unordered_set>

#include <jelly/BlobNode.h>
#include <jelly/ErrorUtils.h>
#include <jelly/DefaultHost.h>
#include <jelly/LockNode.h>
#include <jelly/UIntLock.h>
#include <jelly/UIntKey.h>

#include "Config.h"
#include "NodeTest.h"
#include "TestDefaultHost.h"

namespace jelly
{

	namespace Test
	{
		
		namespace
		{

			struct UInt32Blob
			{
				UInt32Blob(uint32_t aValue = 0) : m_value(aValue), m_loaded(aValue != 0) {}

				bool		
				Write(
					IWriter*						aWriter,
					const Compression::IProvider*	/*aCompression*/) const
				{ 
					JELLY_ASSERT(m_loaded); 
					
					if(aWriter->Write(&m_value, sizeof(m_value)) != sizeof(m_value))
						return false;

					uint32_t fluff[10];
					for (uint32_t i = 0; i < 10; i++)
						fluff[i] = m_value * i;

					if(aWriter->Write(fluff, sizeof(fluff)) != sizeof(fluff))
						return false;

					return true;
				}
				
				bool		
				Read(
					IReader*						aReader,
					const Compression::IProvider*	/*aCompression*/) 
				{ 
					m_loaded = true;

					if(aReader->Read(&m_value, sizeof(m_value)) != sizeof(m_value))
						return false;

					uint32_t fluff[10];
					if (aReader->Read(fluff, sizeof(fluff)) != sizeof(fluff))
						return false;

					for (uint32_t i = 0; i < 10; i++)
						JELLY_ASSERT(fluff[i] == m_value * i);

					return true;
				}

				bool		operator==(const UInt32Blob& aOther) const { JELLY_ASSERT(m_loaded && aOther.m_loaded); return m_value == aOther.m_value; }				
				bool		IsSet() const { return m_loaded; }
				void		Reset() { JELLY_ASSERT(m_loaded); m_value = 0; m_loaded = false; }
				size_t		GetSize() const { return sizeof(uint32_t) + 10 * sizeof(uint32_t); }
				size_t		GetStoredSize() const { return GetSize(); }
				void		Move(UInt32Blob& aOther) { *this = aOther; }
				void		Copy(const UInt32Blob& aOther) { *this = aOther; }

				uint32_t	m_value;
				bool		m_loaded;
			};

			struct UInt32KeyHasher
			{
				std::size_t 
				operator()(
					const UIntKey<uint32_t>& aKey) const 
				{ 
					return std::hash<uint32_t>{}(aKey.m_value);
				}
			};

			typedef BlobNode<
				UIntKey<uint32_t>,
				UInt32Blob,
				UInt32KeyHasher> BlobNodeType;

			typedef LockNode<
				UIntKey<uint32_t>,
				UIntLock<uint32_t>,
				UInt32KeyHasher> LockNodeType;

			template <typename _ItemType>
			void
			_VerifyFileStreamReader(
				Compression::IProvider*										aCompression,
				IFileStreamReader*											aFileStreamReader,
				const std::vector<_ItemType>&								aExpected)
			{
				std::unique_ptr<IFileStreamReader> f(aFileStreamReader);
				JELLY_ASSERT(f);

				for (const _ItemType& expected : aExpected)
				{
					JELLY_ASSERT(!f->IsEnd());
					_ItemType item;
					JELLY_ASSERT(item.Read(f.get(), aCompression));
					JELLY_ASSERT(item.Compare(&expected));
				}

				JELLY_ASSERT(f->IsEnd());
			}

			template <typename _ItemType>
			void
			_VerifyWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<_ItemType>&								aExpected)
			{
				_VerifyFileStreamReader<_ItemType>(NULL, aHost->ReadWALStream(aNodeId, aId), aExpected);
			}

			template <typename _ItemType>
			void
			_VerifyStore(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId,
				const std::vector<_ItemType>&								aExpected)
			{
				_VerifyFileStreamReader<_ItemType>(aHost->GetCompressionProvider(), aHost->ReadStoreStream(aNodeId, aId), aExpected);
			}

			void
			_VerifyNoWAL(
				DefaultHost*												aHost,
				uint32_t													aNodeId,
				uint32_t													aId)
			{
				std::unique_ptr<IFileStreamReader> f(aHost->ReadWALStream(aNodeId, aId));
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
				(*aThreadCounter)++;

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
				(*aThreadCounter)++;

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
							blobNodeId = req.m_blobNodeIds & 0x000000FF;

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
						req.m_blobNodeIds = { 1 };
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
				(*aThreadCounter)++;

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
								threads.push_back(std::thread(_HammerTestThreadWriteBlobs, &lockNode, &blobNode, &stopEvent, lockId++, aKeyCount, &lockCounter, &setCounter, &getCounter, validKeySet.get(), &threadCounter));

							// And some threads for reading blobs
							for (uint32_t i = 0; i < 100; i++)
								threads.push_back(std::thread(_HammerTestThreadReadBlobs, &blobNode, &stopEvent, aKeyCount, &lockCounter, &setCounter, &getCounter, validKeySet.get(), &threadCounter));
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
							printf("%d locks, %d sets, %d gets, total resident blob size: %llu", locks, sets, gets, blobNode.GetTotalResidentBlobSize());

							totalSets += (uint32_t)sets;
							totalGets += (uint32_t)gets;

							if(validKeySet)
								printf(", keys: %llu\n", validKeySet->GetCount());
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
								lockNode.PerformCompaction();
								lockNode.ApplyCompactionResult();
							}
							if (seconds % 8 == 0)
							{
								printf("blob: perform compaction...\n");
								blobNode.PerformCompaction();
								blobNode.ApplyCompactionResult();
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

				_VerifyWAL<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 1, 123 } });
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
				_VerifyWAL<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 1, { { 1, 2, 789 } });
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 2, 789 } });

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

				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 0, { { 1, 2, 789 } });
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 1, { { 1, 3, 1000 }, {2, 1, 1234 } });
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 2, { { 1, 4, 1001 } });

				// Perform compaction, which should remove two of the store files and create a new one
				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 0;

					BlobNodeType blobNode(aHost, 0, config);
					_VerifyResidentKeys(&blobNode, { });
					blobNode.PerformCompaction();
					blobNode.ApplyCompactionResult();
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
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 2, { { 1, 4, 1001 } });
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 0, 3, { { 1, 3, 1000 }, { 2, 1, 1234 } });
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

				_VerifyWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, 0xFFFFFFFF } });
				
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

				_VerifyWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0, { { 1, 1, 123, UINT32_MAX, 0xFFFFFFFF } });
				_VerifyWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 1, { { 1, 2, 0, 1, 0xFF030201 } });

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
				_VerifyWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 1, { { 1, 2, 0, 1, 0xFF030201 } });
				_VerifyWAL<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 2,
				{ 
					{ 1, 3, 456, 1, 0xFF030201 },
					{ 3, 1, 456, UINT32_MAX, 0xFFFFFFFF },
					{ 2, 1, 456, UINT32_MAX, 0xFFFFFFFF }
				});
				_VerifyStore<LockNodeItem<UIntKey<uint32_t>, UIntLock<uint32_t>>>(aHost, 0, 0,
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
			_TestBlobNodeMemoryLimit(
				DefaultHost*		aHost)
			{
				aHost->DeleteAllFiles(UINT32_MAX);

				{
					BlobNodeType::Config config;
					config.m_maxResidentBlobSize = 5 * (sizeof(uint32_t) * 11); // Each blob is 1 uint32_t + 10 more

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

				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 2, 0, { { 1, 1, 101 } });
				_VerifyStore<BlobNodeItem<UIntKey<uint32_t>, UInt32Blob>>(aHost, 2, 1, { { 1, 2, 102 } });

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