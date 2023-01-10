#pragma once

#include <jelly/Blob.h>
#include <jelly/BlobNode.h>
#include <jelly/UIntKey.h>

#include "Stats.h"
#include "Timer.h"

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

			NUM_STATS
		};

		static void
		InitStats(
			Stats&							aStats)
		{
			aStats.Init(NUM_STATS);
		}

		static void
		PrintStats(
			const Stats&					aStats,
			const std::vector<uint32_t>&	aStateCounters)
		{
			aStats.Print(Stats::TYPE_SAMPLE, STAT_PROCESSED_REQUESTS, "PROCESSED_REQUESTS");

			Stats::PrintStateCounter("INIT", STATE_INIT, aStateCounters);
			Stats::PrintStateCounter("RUNNING", STATE_RUNNING, aStateCounters);
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
			, m_accumRequestCount(0)
			, m_hasNode(false)
		{
		}
	
		~NodeServer()
		{

		}

		void			
		Update(
			Stats&			aStats)
		{
			switch(m_state)
			{
			case STATE_INIT:
				{
					typename _NodeType::Config config;

					m_node = std::make_unique<_NodeType>(&m_network->m_host, m_id, config);
					m_hasNode = true;

					m_flushPendingWALsTimer.SetTimeout(1000);
					m_performCompactionTimer.SetTimeout(60 * 1000);

					m_state = STATE_RUNNING;
				}
				break;

			case STATE_RUNNING:
				{
					uint32_t numProcessedRequests = m_node->ProcessRequests();

					aStats.Sample(STAT_PROCESSED_REQUESTS, numProcessedRequests);
				
					m_accumRequestCount += numProcessedRequests;

					if (m_accumRequestCount > 0 && m_flushPendingWALsTimer.HasExpired())
					{
						m_node->FlushPendingWAL(0);

						m_accumRequestCount = 0;
					}

					//if(m_performCompactionTimer.HasExpired())
					//{
					//	m_node->PerformCompaction();
					//}
				}
				break;
			}
		}

		void		
		UpdateStateCounters(
			std::vector<uint32_t>&	aOut)
		{
			JELLY_ASSERT((size_t)m_state < aOut.size());
			aOut[m_state]++;
		}

		// Data access
		_NodeType*		GetNode() { return m_hasNode ? m_node.get() : NULL; }

	private:

		Network*						m_network;
		uint32_t						m_id;

		std::atomic_bool				m_hasNode;
		std::unique_ptr<_NodeType>		m_node;

		uint32_t						m_accumRequestCount;

		Timer							m_flushPendingWALsTimer;
		Timer							m_performCompactionTimer;

		Stats							m_stats;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_RUNNING,

			NUM_STATES
		};

		State							m_state;
	};
	
}