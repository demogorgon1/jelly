#pragma once

#include "SharedWorkQueue.h"
#include "StateTimeSampler.h"
#include "Stats.h"

namespace jelly::Test::Sim
{

	class Network;

	enum NodeServerType
	{
		NODE_SERVER_TYPE_BLOB,
		NODE_SERVER_TYPE_LOCK
	};

	template <typename _NodeType, NodeServerType _Type>
	class NodeServer
	{
	public:
		static uint32_t
		GetNumStates()
		{
			return NUM_STATES;
		}

		static uint32_t
		GetStateNumStatsId(
			uint32_t											aState)
		{
			// IMPORTANT: must match State enum
			static const uint32_t IDS[] =
			{
				_Type == NODE_SERVER_TYPE_LOCK ? Stats::ID_LS_INIT_NUM : Stats::ID_BS_INIT_NUM,
				_Type == NODE_SERVER_TYPE_LOCK ? Stats::ID_LS_RUNNING_NUM : Stats::ID_BS_RUNNING_NUM,
			};
			static_assert(sizeof(IDS) == sizeof(uint32_t) * (size_t)NUM_STATES);
			JELLY_ALWAYS_ASSERT(aState < (uint32_t)NUM_STATES);
			return IDS[aState];
		}

		NodeServer(
			Network*											aNetwork,
			SharedWorkQueue*									aSharedWorkQueue,
			IHost*												aHost,
			uint32_t											aId)
			: m_network(aNetwork)
			, m_sharedWorkQueue(aSharedWorkQueue)
			, m_host(aHost)
			, m_id(aId)
			, m_state(STATE_INIT)
			, m_hasNode(false)
			, m_stateTimeSampler(NUM_STATES)
			, m_processRequestsTimer(50)
		{
			if constexpr(_Type == NODE_SERVER_TYPE_LOCK)
			{
				m_stateTimeSampler.DefineState(STATE_INIT, Stats::ID_LS_INIT_TIME, Stats::ID_LS_INIT_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_RUNNING, Stats::ID_LS_RUNNING_TIME, Stats::ID_LS_RUNNING_CUR_TIME);
			}
			else
			{
				m_stateTimeSampler.DefineState(STATE_INIT, Stats::ID_BS_INIT_TIME, Stats::ID_BS_INIT_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_RUNNING, Stats::ID_BS_RUNNING_TIME, Stats::ID_BS_RUNNING_CUR_TIME);
			}
		}
	
		~NodeServer()
		{

		}

		void			
		Update()
		{
			switch(m_state)
			{
			case STATE_INIT:
				{
					m_node = std::make_unique<_NodeType>(m_host, m_id);
					m_hasNode = true;

					m_housekeepingAdvisor = std::make_unique<HousekeepingAdvisor<_NodeType>>(m_host, m_node.get());

					m_state = STATE_RUNNING;
				}
				break;

			case STATE_RUNNING:
				{	
					// Process incoming requests
					if(m_processRequestsTimer.HasExpired())
						m_node->ProcessRequests();
					
					// Apply compaction results, if any
					{
						std::vector<std::unique_ptr<CompactionResultType>> compactionResults;
						
						{
							std::lock_guard lock(m_compactionResultsLock);
							if(m_compactionResults.size() > 0)
								compactionResults = std::move(m_compactionResults);
						}

						for(std::unique_ptr<CompactionResultType>& compactionResult : compactionResults)
							m_node->ApplyCompactionResult(compactionResult.get());
					}
				
					// Do what housekeeping advisor tells us
					m_housekeepingAdvisor->Update([&](
						const HousekeepingAdvisor<_NodeType>::Event& aEvent)
					{
						switch(aEvent.m_type)
						{
						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_WAL:
							m_node->FlushPendingWAL(aEvent.m_concurrentWALIndex);
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_STORE:
							m_node->FlushPendingStore();
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_CLEANUP_WALS:
							m_node->CleanupWALs();
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_PERFORM_COMPACTION:
							m_sharedWorkQueue->PostWork([&, compactionJob = aEvent.m_compactionJob]()
							{
								std::unique_ptr<CompactionResultType> compactionResult(m_node->PerformCompaction(compactionJob));

								std::lock_guard lock(m_compactionResultsLock);
								m_compactionResults.push_back(std::move(compactionResult));
							});								
							break;

						default:
							JELLY_ALWAYS_ASSERT(false);
						}
					});						
				}
				break;

			default:
				JELLY_ALWAYS_ASSERT(false);
				break;
			}
		}

		void		
		UpdateStateStatistics(
			std::vector<uint32_t>& aStateCounters)
		{
			JELLY_ALWAYS_ASSERT((size_t)m_state < aStateCounters.size());
			
			aStateCounters[m_state]++;

			m_stateTimeSampler.EmitCurrentStateTime(m_host->GetStats(), std::chrono::steady_clock::now());
		}

		// Data access
		_NodeType*		GetNode() { return m_hasNode ? m_node.get() : NULL; }

	private:

		Network*													m_network;
		SharedWorkQueue*											m_sharedWorkQueue;
		IHost*														m_host;
		uint32_t													m_id;

		std::atomic_bool											m_hasNode;
		std::unique_ptr<_NodeType>									m_node;
		std::unique_ptr<HousekeepingAdvisor<_NodeType>>				m_housekeepingAdvisor;

		typedef _NodeType::CompactionResultType CompactionResultType;
		std::mutex													m_compactionResultsLock;
		std::vector<std::unique_ptr<CompactionResultType>>			m_compactionResults;

		Timer														m_processRequestsTimer;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_RUNNING,

			NUM_STATES
		};

		State														m_state;
		StateTimeSampler											m_stateTimeSampler;

		void
		_SetState(
			State									aState)
		{
			JELLY_ALWAYS_ASSERT(m_state != aState);
			m_stateTimeSampler.ChangeState(m_host->GetStats(), aState);
			m_state = aState;
		}
	};
	
}