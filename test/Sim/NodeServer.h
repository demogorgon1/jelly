#pragma once

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
		enum Stat : uint32_t
		{
			STAT_PROCESSED_REQUESTS,
			STAT_INIT_TIME,
			STAT_RUNNING_TIME,

			NUM_STATS
		};

		static void
		InitCSV(
			const char*							aColumnPrefix,
			CSVOutput*							aCSV)
		{
			Stats::InitCSVColumn(aColumnPrefix, "PROCESS_REQUESTS", aCSV);

			Stats::InitStateInfoCSV(aColumnPrefix, "INIT", aCSV);
			Stats::InitStateInfoCSV(aColumnPrefix, "RUNNING", aCSV);
		}

		static void
		InitStats(
			Stats&								aStats)
		{
			aStats.Init(NUM_STATS);
		}

		static void
		PrintStats(
			const Stats&						aStats,
			const std::vector<Stats::Entry>&	aStateInfo,
			CSVOutput*							aCSV,
			const char*							aCSVColumnPrefix,
			const Config*						aConfig)
		{
			aStats.Print(Stats::TYPE_SAMPLE, STAT_PROCESSED_REQUESTS, "PROCESSED_REQUESTS", aCSVColumnPrefix, aCSV, aConfig);

			Stats::PrintStateInfo("INIT", STATE_INIT, aStateInfo, aStats, STAT_INIT_TIME, aCSVColumnPrefix, aCSV, aConfig);
			Stats::PrintStateInfo("RUNNING", STATE_RUNNING, aStateInfo, aStats, STAT_RUNNING_TIME, aCSVColumnPrefix, aCSV, aConfig);
		}

		static uint32_t
		GetNumStates()
		{
			return NUM_STATES;
		}

		NodeServer(
			Network*						aNetwork,
			uint32_t						aId)
			: m_network(aNetwork)
			, m_id(aId)
			, m_state(STATE_INIT)
			, m_hasNode(false)
		{
			m_stateTimeStamp = std::chrono::steady_clock::now();
		}
	
		~NodeServer()
		{

		}

		void			
		Update(
			IHost*			aHost,
			Stats&			aStats)
		{
			switch(m_state)
			{
			case STATE_INIT:
				{
					typename _NodeType::Config config;

					m_node = std::make_unique<_NodeType>(aHost, m_id, config);
					m_hasNode = true;

					typename HousekeepingAdvisor<_NodeType>::Config housekeepingAdvisorConfig;

					m_housekeepingAdvisor = std::make_unique<HousekeepingAdvisor<_NodeType>>(aHost, m_node.get(), housekeepingAdvisorConfig);

					m_state = STATE_RUNNING;
				}
				break;

			case STATE_RUNNING:
				{
					uint32_t numProcessedRequests = m_node->ProcessRequests();

					aStats.Sample(STAT_PROCESSED_REQUESTS, numProcessedRequests);
				
					m_housekeepingAdvisor->Update([&](
						const HousekeepingAdvisor<_NodeType>::Event& aEvent)
					{
						switch(aEvent.m_type)
						{
						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_WAL:
							m_node->FlushPendingWAL(aEvent.m_u.m_concurrentWALIndex);
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_FLUSH_PENDING_STORE:
							m_node->FlushPendingStore();
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_CLEANUP_WALS:
							m_node->CleanupWALs();
							break;

						case HousekeepingAdvisor<_NodeType>::Event::TYPE_PERFORM_COMPACTION:
							// FIXME: perform compaction
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
		UpdateStateInfo(
			Stats&						aStats,
			std::vector<Stats::Entry>&	aOut)
		{
			aStats.AddAndResetEntry(STAT_INIT_TIME, m_stateTimes[STATE_INIT]);
			aStats.AddAndResetEntry(STAT_RUNNING_TIME, m_stateTimes[STATE_RUNNING]);

			JELLY_ASSERT((size_t)m_state < aOut.size());
			aOut[m_state].Sample((uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_stateTimeStamp).count());
		}

		// Data access
		_NodeType*		GetNode() { return m_hasNode ? m_node.get() : NULL; }

	private:

		Network*											m_network;
		uint32_t											m_id;

		std::atomic_bool									m_hasNode;
		std::unique_ptr<_NodeType>							m_node;
		std::unique_ptr<HousekeepingAdvisor<_NodeType>>		m_housekeepingAdvisor;

		Stats												m_stats;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_RUNNING,

			NUM_STATES
		};

		State												m_state;
		std::chrono::time_point<std::chrono::steady_clock>	m_stateTimeStamp;
		Stats::Entry										m_stateTimes[NUM_STATES];
	};
	
}