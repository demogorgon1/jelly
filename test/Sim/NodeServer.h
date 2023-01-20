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
			Network*						aNetwork,
			uint32_t						aId)
			: m_network(aNetwork)
			, m_id(aId)
			, m_state(STATE_INIT)
			, m_hasNode(false)
			, m_stateTimeSampler(NUM_STATES)
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
					typename _NodeType::Config config;

					m_node = std::make_unique<_NodeType>(&m_network->m_host, m_id, config);
					m_hasNode = true;

					typename HousekeepingAdvisor<_NodeType>::Config housekeepingAdvisorConfig;

					m_housekeepingAdvisor = std::make_unique<HousekeepingAdvisor<_NodeType>>(&m_network->m_host, m_node.get(), housekeepingAdvisorConfig);

					m_state = STATE_RUNNING;
				}
				break;

			case STATE_RUNNING:
				{
					m_node->ProcessRequests();
				
					m_housekeepingAdvisor->Update([&](
						const HousekeepingAdvisor<_NodeType>::Event& aEvent)
					{
						switch(aEvent.m_type)
						{
						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_WAL:
							{
								size_t itemCount = m_node->FlushPendingWAL(aEvent.m_concurrentWALIndex);
								JELLY_UNUSED(itemCount);
							}
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_STORE:
							{
								size_t itemCount = m_node->FlushPendingStore();

								if (itemCount > 0)
									printf("[%u] FlushPendingStore: %llu\n", m_id, itemCount);
							}
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_CLEANUP_WALS:
							{
								size_t walCount = m_node->CleanupWALs();

								if (walCount > 0)
									printf("[%u] CleanupWALs: %llu\n", m_id, walCount);
							}
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_PERFORM_COMPACTION:
							{
								std::unique_ptr<typename _NodeType::CompactionResultType> compactionResult(m_node->PerformCompaction(aEvent.m_compactionJob));
								m_node->ApplyCompactionResult(compactionResult.get());

								printf("[%u] PerformCompaction\n", m_id);
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

			m_stateTimeSampler.EmitCurrentStateTime(m_network->m_host.GetStats(), std::chrono::steady_clock::now());
		}

		// Data access
		_NodeType*		GetNode() { return m_hasNode ? m_node.get() : NULL; }

	private:

		Network*											m_network;
		uint32_t											m_id;

		std::atomic_bool									m_hasNode;
		std::unique_ptr<_NodeType>							m_node;
		std::unique_ptr<HousekeepingAdvisor<_NodeType>>		m_housekeepingAdvisor;

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
			m_stateTimeSampler.ChangeState(m_network->m_host.GetStats(), aState);
			m_state = aState;
		}
	};
	
}