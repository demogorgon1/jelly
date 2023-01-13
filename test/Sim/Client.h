#pragma once

#include "CSVOutput.h"
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
		InitCSV(
			const char*											aColumnPrefix,
			CSVOutput*											aCSV)
		{
			Stats::InitStateInfoCSV(aColumnPrefix, "INIT", aCSV);
			Stats::InitStateInfoCSV(aColumnPrefix, "WAITING_TO_CONNECT", aCSV);
			Stats::InitStateInfoCSV(aColumnPrefix, "WAITING_FOR_CONNECTION", aCSV);
			Stats::InitStateInfoCSV(aColumnPrefix, "CONNECTED", aCSV);
			Stats::InitStateInfoCSV(aColumnPrefix, "DISCONNECTED", aCSV);
		}

		static void
		InitStats(
			Stats&												aStats)
		{
			aStats.Init(NUM_STATS);
		}

		static void
		PrintStats(
			const Stats&										aStats,
			const std::vector<Stats::Entry>&					aStateInfo,
			CSVOutput*											aCSV,
			const char*											aCSVColumnPrefix,
			const Config*										aConfig)
		{
			Stats::PrintStateInfo("INIT", STATE_INIT, aStateInfo, aStats, STAT_INIT_TIME, aCSVColumnPrefix, aCSV, aConfig);
			Stats::PrintStateInfo("WAITING_TO_CONNECT", STATE_WAITING_TO_CONNECT, aStateInfo, aStats, STAT_WAITING_TO_CONNECT_TIME, aCSVColumnPrefix, aCSV, aConfig);
			Stats::PrintStateInfo("WAITING_FOR_CONNECTION", STATE_WAITING_FOR_CONNECTION, aStateInfo, aStats, STAT_WAITING_FOR_CONNECTION_TIME, aCSVColumnPrefix, aCSV, aConfig);
			Stats::PrintStateInfo("CONNECTED", STATE_CONNECTED, aStateInfo, aStats, STAT_CONNECTED_TIME, aCSVColumnPrefix, aCSV, aConfig);
			Stats::PrintStateInfo("DISCONNECTED", STATE_DISCONNECTED, aStateInfo, aStats, STAT_DISCONNECTED_TIME, aCSVColumnPrefix, aCSV, aConfig);
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
		void		UpdateStateInfo(
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
		std::chrono::time_point<std::chrono::steady_clock>	m_startTimeStamp;

		void		_SetState(
						State									aState);
	};
	
}