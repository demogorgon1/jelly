#pragma once

#include <random>

namespace jelly
{

	namespace
	{

		const char*
		_ResultToString(
			Result									aResult)
		{
			switch(aResult)
			{
			case RESULT_NONE:				return "NONE";
			case RESULT_OK:					return "OK";
			case RESULT_CANCELED:			return "CANCELED";
			case RESULT_ALREADY_LOCKED:		return "ALREADY_LOCKED";
			case RESULT_NOT_LOCKED:			return "NOT_LOCKED";
			case RESULT_DOES_NOT_EXIST:		return "DOES_NOT_EXIST";
			case RESULT_OUTDATED:			return "OUTDATED";
			}
			JELLY_ASSERT(false);
			return "";
		}

	}
	
	namespace Test
	{
		
		template <
			typename _LockNodeType, 
			typename _LockNodeRequestType,
			typename _BlobNodeType,
			typename _BlobNodeRequestType>
		class StepThread
		{
		public:
			StepThread(
				const char*							aThreadName,
				IHost*								aHost,
				std::unique_ptr<_LockNodeType>&		aLockNode,
				std::unique_ptr<_BlobNodeType>&		aBlobNode,
				std::mt19937&						aRandom)
				: m_host(aHost)
				, m_threadName(aThreadName)
				, m_lockNode(aLockNode)
				, m_blobNode(aBlobNode)
				, m_nextStepIndex(0)
				, m_key(0)
				, m_lockId(0)
				, m_blobSeq(0)
				, m_random(aRandom)
				, m_verbose(false)
			{

			}

			~StepThread()
			{

			}

			void
			SetVerbose(
				bool								aVerbose)
			{
				m_verbose = aVerbose;
			}

			void
			SetVerboseFilter(
				const char*							aVerboseFilter)
			{
				m_verboseFilter = aVerboseFilter;
			}

			void
			Step(
				uint32_t							aStepId)
			{
				m_stepId = aStepId;

				if(!_UpdateRequests())
				{
					JELLY_ASSERT(m_nextStepIndex < m_steps.size());

					Entry& entry = m_steps[m_nextStepIndex];

					if(entry.m_flags & Entry::FLAG_REQUIRE_BLOB_NODE)
					{
						if(!m_blobNode)
							return;
					}

					if (entry.m_flags & Entry::FLAG_REQUIRE_LOCK_NODE)
					{
						if (!m_lockNode)
							return;
					}

					m_nextStepIndex = (m_nextStepIndex + 1) % m_steps.size();

					if(entry.m_propability != 0)
					{
						if(m_random() % 100 < entry.m_propability)
							return;
					}

					entry.m_function();
				}
			}

			void
			RestartLockNode()
			{
				_AddStep(__FUNCTION__, 0, 0, [&]()
				{
					typename _LockNodeType::Config config;
						
					if (!m_lockNode)
					{
						_Message("Start lock node");

						m_lockNode.reset(new _LockNodeType(m_host, 0, config));
					}
					else if(m_random() % 100 < 5)
					{
						_Message("Restart lock node");

						m_lockNode.reset(new _LockNodeType(m_host, 0, config));
					}
				});
			}

			void
			RestartBlobNode(
				size_t			aMaxResidentBlobSize)
			{
				_AddStep(__FUNCTION__, 0, 0, [&, aMaxResidentBlobSize]()
				{
					typename _BlobNodeType::Config config;
					config.m_maxResidentBlobSize = aMaxResidentBlobSize;
						
					if (!m_blobNode)
					{
						_Message("Start blob node: max resident blob size=%llu", aMaxResidentBlobSize);

						m_blobNode.reset(new _BlobNodeType(m_host, 1, config));
					}
					else if (m_random() % 100 < 5)
					{
						_Message("Restart blob node: max resident blob size=%llu", aMaxResidentBlobSize);

						m_blobNode.reset(new _BlobNodeType(m_host, 1, config));
					}
				});
			}

			void
			ProcessLockNodeRequests()
			{
				_AddStep(__FUNCTION__, 0, 0, [&]()
				{
					_Message("Process lock node requests");

					JELLY_ASSERT(m_lockNode);
					size_t count = m_lockNode->ProcessRequests();

					_Message("(%llu requests processed)", count);
				});
			}

			void
			FlushPendingLockNodeWAL()
			{
				_AddStep(__FUNCTION__, 0, 10, [&]()
				{
					_Message("Flush pending lock node WAL");

					JELLY_ASSERT(m_lockNode);
					m_lockNode->FlushPendingWAL(0);
				});
			}

			void
			FlushPendingLockNodeStore()
			{
				_AddStep(__FUNCTION__, 0, 2, [&]()
				{
					_Message("Flush pending lock node store");

					JELLY_ASSERT(m_lockNode);
					m_lockNode->FlushPendingStore();
				});
			}

			void
			CleanupLockNodeWALs()
			{
				_AddStep(__FUNCTION__, 0, 2, [&]()
				{
					_Message("Cleanup lock node WALs");

					JELLY_ASSERT(m_lockNode);
					m_lockNode->CleanupWALs();
				});
			}

			void
			ProcessBlobNodeRequests()
			{
				_AddStep(__FUNCTION__, 0, 0, [&]()
				{
					_Message("Process blob node requests");

					JELLY_ASSERT(m_blobNode);
					size_t count = m_blobNode->ProcessRequests();

					_Message("(%llu requests processed)", count);
				});
			}

			void
			FlushPendingBlobNodeWAL()
			{
				_AddStep(__FUNCTION__, 0, 10, [&]()
				{
					_Message("Flush pending blob node WAL");

					JELLY_ASSERT(m_blobNode);
					m_blobNode->FlushPendingWAL(0);
				});
			}

			void
			FlushPendingBlobNodeStore()
			{
				_AddStep(__FUNCTION__, 0, 2, [&]()
				{
					_Message("Flush pending blob node store");

					JELLY_ASSERT(m_blobNode);
					m_blobNode->FlushPendingStore();
				});
			}

			void
			CleanupBlobNodeWALs()
			{
				_AddStep(__FUNCTION__, 0, 2, [&]()
				{
					_Message("Cleanup blob node WALs");

					JELLY_ASSERT(m_blobNode);
					m_blobNode->CleanupWALs();
				});
			}

			void
			SetLockId(
				uint32_t		aLockId)
			{
				_AddStep(__FUNCTION__, 0, 0, [&, aLockId]()
				{
					m_lockId = aLockId;
				});
			}

			void
			SetKey(
				uint32_t		aKey)
			{
				_AddStep(__FUNCTION__, 0, 0, [&, aKey]()
				{
					m_key = aKey;
				});
			}

			void
			Lock()
			{
				_AddStep(__FUNCTION__, Entry::FLAG_REQUIRE_LOCK_NODE, 0, [&]()
				{
					_Message("Lock: key=%u lock id=%u", m_key, m_lockId);

					JELLY_ASSERT(m_lockNode);
					JELLY_ASSERT(!m_lockRequest);

					m_lockRequest = std::make_unique<_LockNodeRequestType>();
					m_lockRequest->SetKey(m_key);
					m_lockRequest->SetLock(m_lockId);
					m_lockNode->Lock(m_lockRequest.get());
				});				
			}

			void
			Unlock()
			{
				_AddStep(__FUNCTION__, Entry::FLAG_REQUIRE_LOCK_NODE, 0, [&]()
				{
					_Message("Unlock: key=%u lock id=%u", m_key, m_lockId);

					JELLY_ASSERT(m_lockNode);
					JELLY_ASSERT(!m_unlockRequest);

					m_unlockRequest = std::make_unique<_LockNodeRequestType>();
					m_unlockRequest->SetKey(m_key);
					m_unlockRequest->SetLock(m_lockId);
					m_unlockRequest->SetBlobSeq(m_blobSeq);
					m_lockNode->Unlock(m_unlockRequest.get());
				});				
			}

			void
			Get()
			{
				_AddStep(__FUNCTION__, Entry::FLAG_REQUIRE_BLOB_NODE, 0, [&]()
				{
					_Message("Get: key=%u", m_key);

					JELLY_ASSERT(m_blobNode);
					JELLY_ASSERT(!m_getRequest);

					m_getRequest = std::make_unique<_BlobNodeRequestType>();
					m_getRequest->SetKey(m_key);

					if(m_blobSeq != UINT32_MAX)
						m_getRequest->SetSeq(m_blobSeq);

					m_blobNode->Get(m_getRequest.get());
				});								
			}

			void
			Set()
			{
				_AddStep(__FUNCTION__, Entry::FLAG_REQUIRE_BLOB_NODE, 0, [&]()
				{
					_Message("Set: key=%u blob_seq=%u", m_key, m_blobSeq);

					JELLY_ASSERT(m_blobNode);
					JELLY_ASSERT(!m_setRequest);

					m_setRequest = std::make_unique<_BlobNodeRequestType>();
					m_setRequest->SetKey(m_key);
					m_setRequest->SetSeq(++m_blobSeq);
					m_setRequest->SetBlob(m_key);
					m_blobNode->Set(m_setRequest.get());
				});								
			}

		private:

			std::string								m_threadName;
			IHost*									m_host;
			std::unique_ptr<_LockNodeType>&			m_lockNode;
			std::unique_ptr<_BlobNodeType>&			m_blobNode;
			std::mt19937&							m_random;

			size_t									m_nextStepIndex;
			uint32_t								m_stepId;

			struct Entry
			{
				enum Flag : uint32_t
				{
					FLAG_REQUIRE_LOCK_NODE = 0x00000001,
					FLAG_REQUIRE_BLOB_NODE = 0x00000001
				};

				std::string							m_description;
				std::function<void()>				m_function;
				uint32_t							m_flags;
				uint32_t							m_propability;
			};

			std::vector<Entry>						m_steps;

			uint32_t								m_lockId;
			uint32_t								m_key;
			uint32_t								m_blobSeq;

			std::unique_ptr<_LockNodeRequestType>	m_lockRequest;
			std::unique_ptr<_LockNodeRequestType>	m_unlockRequest;
			std::unique_ptr<_BlobNodeRequestType>	m_getRequest;
			std::unique_ptr<_BlobNodeRequestType>	m_setRequest;

			bool									m_verbose;
			std::string								m_verboseFilter;

			void
			_AddStep(
				const char*					aDescription,
				uint32_t					aFlags,
				uint32_t					aPropability,
				std::function<void()>		aFunction)
			{			
				m_steps.push_back(Entry{aDescription, aFunction, aFlags, aPropability});
			}

			bool
			_UpdateRequests()
			{
				if(m_lockRequest)
				{
					if(m_lockRequest->IsCompleted())
					{
						_Message("Completed lock request: %s (key=%u)", _ResultToString(m_lockRequest->GetResult()), m_lockRequest->GetKey());

						switch(m_lockRequest->GetResult())
						{
						case RESULT_ALREADY_LOCKED:
						case RESULT_CANCELED:
							m_lockRequest = std::make_unique<_LockNodeRequestType>();
							m_lockRequest->SetKey(m_key);
							m_lockRequest->SetLock(m_lockId);
							m_lockNode->Lock(m_lockRequest.get());
							break;

						default:
							JELLY_ASSERT(m_lockRequest->GetResult() == RESULT_OK);
							m_blobSeq = m_lockRequest->GetBlobSeq();
							m_lockRequest.reset();
							break;
						}
					}
				}			

				if (m_unlockRequest)
				{
					if (m_unlockRequest->IsCompleted())
					{
						_Message("Completed unlock request: %s (key=%u)", _ResultToString(m_unlockRequest->GetResult()), m_unlockRequest->GetKey());

						switch(m_unlockRequest->GetResult())
						{
						case RESULT_CANCELED:
							m_unlockRequest = std::make_unique<_LockNodeRequestType>();
							m_unlockRequest->SetKey(m_key);
							m_unlockRequest->SetLock(m_lockId);
							m_unlockRequest->SetBlobSeq(m_blobSeq);
							m_lockNode->Unlock(m_unlockRequest.get());
							break;

						default:
							JELLY_ASSERT(m_unlockRequest->GetResult() == RESULT_OK			// Success
								|| m_unlockRequest->GetResult() == RESULT_NOT_LOCKED		// Was already unlocked
								|| m_unlockRequest->GetResult() == RESULT_ALREADY_LOCKED);	// Was already unlocked AND locked by someone else
							m_unlockRequest.reset();
						}
					}
				}

				if (m_getRequest)
				{
					if (m_getRequest->IsCompleted())
					{
						_Message("Completed get request: %s (key=%u)", _ResultToString(m_getRequest->GetResult()), m_getRequest->GetKey());

						switch(m_getRequest->GetResult())
						{
						case RESULT_OK:
							JELLY_ASSERT(m_getRequest->GetBlob() == m_key);
							m_getRequest.reset();
							break;

						case RESULT_CANCELED:
							m_getRequest = std::make_unique<_BlobNodeRequestType>();
							m_getRequest->SetKey(m_key);

							if (m_blobSeq != UINT32_MAX)
								m_getRequest->SetSeq(m_blobSeq);

							m_blobNode->Get(m_getRequest.get());
							break;

						default:
							JELLY_ASSERT(m_getRequest->GetResult() == RESULT_DOES_NOT_EXIST);
							m_getRequest.reset();
							break;
						}
					}
				}

				if (m_setRequest)
				{
					if (m_setRequest->IsCompleted())
					{
						_Message("Completed set request: %s (key=%u)", _ResultToString(m_setRequest->GetResult()), m_setRequest->GetKey());

						switch (m_setRequest->GetResult())
						{
						case RESULT_CANCELED:
							m_setRequest = std::make_unique<_BlobNodeRequestType>();
							m_setRequest->SetKey(m_key);
							m_setRequest->SetSeq(++m_blobSeq);
							m_setRequest->SetBlob(m_key);
							m_blobNode->Set(m_setRequest.get());
							break;

						default:
							JELLY_ASSERT(m_setRequest->GetResult() == RESULT_OK);
							m_setRequest.reset();
						}
					}
				}

				return m_lockRequest || m_unlockRequest || m_getRequest || m_setRequest;
			}

			void
			_Message(
				const char*					aFormat,
				...)
			{
				if(m_verbose)
				{
					char buffer[4096];
					JELLY_STRING_FORMAT_VARARGS(buffer, sizeof(buffer), aFormat);

					if(m_verboseFilter.empty() || strstr(buffer, m_verboseFilter.c_str()) != NULL)						
						printf("[%u][%s] %s\n", m_stepId, m_threadName.c_str(), buffer);
				}
			}
		};

	}

}