#pragma once

#include "CSVOutput.h"
#include "GameServer.h"
#include "StateTimeSampler.h"

namespace jelly::Test::Sim
{

	class Network;

	class Client
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
				Stats::ID_C_INIT_NUM,
				Stats::ID_C_WAITING_TO_CONNECT_NUM, 
				Stats::ID_C_WAITING_FOR_CONNECTION_NUM, 
				Stats::ID_C_CONNECTED_NUM, 
				Stats::ID_C_DISCONNECTED_NUM
			};
			static_assert(sizeof(IDS) == sizeof(uint32_t) * (size_t)NUM_STATES);
			JELLY_ASSERT(aState < (uint32_t)NUM_STATES);
			return IDS[aState];
		}

					Client(
						Network*								aNetwork,
						uint32_t								aId);
					~Client();

		void		Update();
		void		UpdateStateStatistics(
						std::vector<uint32_t>&					aStateCounters);

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
		StateTimeSampler									m_stateTimeSampler;

		std::unique_ptr<GameServer::ConnectRequest>			m_gameServerConnectRequest;

		CompletionEvent										m_disconnectEvent;
		std::chrono::time_point<std::chrono::steady_clock>	m_startTimeStamp;

		void		_SetState(
						State									aState);
	};
	
}