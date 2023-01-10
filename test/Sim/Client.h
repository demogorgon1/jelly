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
			const Stats&										/*aStats*/,
			const std::vector<uint32_t>&						aStateCounters)
		{
			Stats::PrintStateCounter("INIT", STATE_INIT, aStateCounters);
			Stats::PrintStateCounter("WAITING_TO_CONNECT", STATE_WAITING_TO_CONNECT, aStateCounters);
			Stats::PrintStateCounter("WAITING_FOR_CONNECTION", STATE_WAITING_FOR_CONNECTION, aStateCounters);
			Stats::PrintStateCounter("CONNECTED", STATE_CONNECTED, aStateCounters);
			Stats::PrintStateCounter("DISCONNECTED", STATE_DISCONNECTED, aStateCounters);
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
						std::vector<uint32_t>&					aOut);

	private:

		Network*									m_network;
		uint32_t									m_id;

		enum State : uint32_t
		{
			STATE_INIT,
			STATE_WAITING_TO_CONNECT,
			STATE_WAITING_FOR_CONNECTION,
			STATE_CONNECTED,
			STATE_DISCONNECTED,

			NUM_STATES
		};

		State										m_state;

		std::unique_ptr<GameServer::ConnectRequest>	m_gameServerConnectRequest;

		CompletionEvent								m_disconnectEvent;
	};
	
}