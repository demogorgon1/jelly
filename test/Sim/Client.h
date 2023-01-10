#pragma once

#include <memory>

#include <stdint.h>

#include "GameServer.h"

namespace jelly::Test::Sim
{

	class Network;

	class Client
	{
	public:
		enum Stat : uint32_t
		{
			STAT_INIT_TIME,
			STAT_WAITING_TO_CONNECT_TIME,
			STAT_WAITING_FOR_CONNECTION_TIME,
			STAT_CONNECTED_TIME,
			STAT_DISCONNECTED_TIME,

			NUM_STATS
		};

		static void
		InitStats(
			Stats&												aStats)
		{
			aStats.Init(NUM_STATS);
		}

		static void
		PrintStats(
			const Stats&										aStats,
			const std::vector<Stats::Entry>&					aStateCounters)
		{
			Stats::PrintStateCounter("INIT", STATE_INIT, aStateCounters, aStats, STAT_INIT_TIME);
			Stats::PrintStateCounter("WAITING_TO_CONNECT", STATE_WAITING_TO_CONNECT, aStateCounters, aStats, STAT_WAITING_TO_CONNECT_TIME);
			Stats::PrintStateCounter("WAITING_FOR_CONNECTION", STATE_WAITING_FOR_CONNECTION, aStateCounters, aStats, STAT_WAITING_FOR_CONNECTION_TIME);
			Stats::PrintStateCounter("CONNECTED", STATE_CONNECTED, aStateCounters, aStats, STAT_CONNECTED_TIME);
			Stats::PrintStateCounter("DISCONNECTED", STATE_DISCONNECTED, aStateCounters, aStats, STAT_DISCONNECTED_TIME);
		}

		static uint32_t
		GetNumStates()
		{
			return NUM_STATES;
		}

					Client(
						Network*								aNetwork,
						uint32_t								aId);
					~Client();

		void		Update(
						IHost*									aHost,
						Stats&									aStats);
		void		UpdateStateCounters(
						Stats&									aStats,
						std::vector<Stats::Entry>&				aOut);

	private:

		Network*											m_network;
		uint32_t											m_id;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_WAITING_TO_CONNECT,
			STATE_WAITING_FOR_CONNECTION,
			STATE_CONNECTED,
			STATE_DISCONNECTED,

			NUM_STATES
		};

		State												m_state;
		std::chrono::time_point<std::chrono::steady_clock>	m_stateTimeStamp;
		Stats::Entry										m_stateTimes[NUM_STATES];

		std::unique_ptr<GameServer::ConnectRequest>			m_gameServerConnectRequest;

		CompletionEvent										m_disconnectEvent;

		void		_SetState(
						State									aState);
	};
	
}