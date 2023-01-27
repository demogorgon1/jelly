#pragma once

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
			JELLY_ASSERT(aState < (uint32_t)NUM_STATES);
			return IDS[aState];
		}

		NodeServer(
			Network*											aNetwork,
			IHost*												aHost,
			uint32_t											aId,
			const typename _NodeType::Config&					aConfig)
			: m_network(aNetwork)
			, m_host(aHost)
			, m_id(aId)
			, m_state(STATE_INIT)
			, m_hasNode(false)
			, m_stateTimeSampler(NUM_STATES)
			, m_config(aConfig)
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
					m_node = std::make_unique<_NodeType>(m_host, m_id, m_config);
					m_hasNode = true;

					typename HousekeepingAdvisor<_NodeType>::Config housekeepingAdvisorConfig;

					m_housekeepingAdvisor = std::make_unique<HousekeepingAdvisor<_NodeType>>(m_host, m_node.get(), housekeepingAdvisorConfig);

					m_state = STATE_RUNNING;
				}
				break;

			case STATE_RUNNING:
				{	
					if(m_processRequestsTimer.HasExpired())
						m_node->ProcessRequests();
				
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
							{
								std::unique_ptr<typename _NodeType::CompactionResultType> compactionResult(m_node->PerformCompaction(aEvent.m_compactionJob));
								m_node->ApplyCompactionResult(compactionResult.get());
							}
							break;

						default:
							JELLY_ASSERT(false);
						}
					});						
				}
				break;

			default:
				JELLY_ASSERT(false);
				break;
			}
		}

		void		
		UpdateStateStatistics(
			std::vector<uint32_t>& aStateCounters)
		{
			JELLY_ASSERT((size_t)m_state < aStateCounters.size());
			
			aStateCounters[m_state]++;

			m_stateTimeSampler.EmitCurrentStateTime(m_host->GetStats(), std::chrono::steady_clock::now());
		}

		// Data access
		_NodeType*		GetNode() { return m_hasNode ? m_node.get() : NULL; }

	private:

		Network*											m_network;
		IHost*												m_host;
		uint32_t											m_id;
		_NodeType::Config									m_config;

		std::atomic_bool									m_hasNode;
		std::unique_ptr<_NodeType>							m_node;
		std::unique_ptr<HousekeepingAdvisor<_NodeType>>		m_housekeepingAdvisor;

		Timer												m_processRequestsTimer;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_RUNNING,

			NUM_STATES
		};

		State												m_state;
		StateTimeSampler									m_stateTimeSampler;

		void
		_SetState(
			State									aState)
		{
			JELLY_ASSERT(m_state != aState);
			m_stateTimeSampler.ChangeState(m_host->GetStats(), aState);
			m_state = aState;
		}
	};
	
}