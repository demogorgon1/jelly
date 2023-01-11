#include "../Config.h"

#include "Client.h"
#include "GameServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	Client::Client(
		Network*	aNetwork,
		uint32_t	aId)
		: m_network(aNetwork)
		, m_id(aId)
		, m_state(STATE_INIT)
	{
		m_stateTimeStamp = std::chrono::steady_clock::now();
	}

	Client::~Client()
	{

	}

	void
	Client::Update(
		IHost*		/*aHost*/,
		Stats&		/*aStats*/)
	{
		switch(m_state)
		{
		case STATE_INIT:
			{
				if (m_network->m_config->m_simClientStartIntervalMS > 0)
				{
					m_startTimeStamp = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_network->m_config->m_simClientStartIntervalMS * m_id);
				}
				else
				{	
					m_startTimeStamp = std::chrono::steady_clock::now();
				}

				_SetState(STATE_WAITING_TO_CONNECT);
			}
			break;

		case STATE_WAITING_TO_CONNECT:
			if(std::chrono::steady_clock::now() > m_startTimeStamp)
			{
				GameServer* gameServer = m_network->PickRandomGameServer();

				m_disconnectEvent.Reset();

				m_gameServerConnectRequest = std::make_unique<GameServer::ConnectRequest>(m_id, &m_disconnectEvent);

				gameServer->Connect(m_gameServerConnectRequest.get());

				_SetState(STATE_WAITING_FOR_CONNECTION);
			}
			break;

		case STATE_WAITING_FOR_CONNECTION:
			if (m_disconnectEvent.Poll())
			{
				_SetState(STATE_DISCONNECTED);
			}
			else if(m_gameServerConnectRequest->m_completed.Poll())
			{
				_SetState(STATE_CONNECTED);
			}
			break;

		case STATE_CONNECTED:
			if(m_disconnectEvent.Poll())
				_SetState(STATE_DISCONNECTED);
			break;

		case STATE_DISCONNECTED:
			break;

		default:
			JELLY_ASSERT(false);
			break;
		}		
	}

	void		
	Client::UpdateStateInfo(
		Stats&									aStats,
		std::vector<Stats::Entry>&				aOut)
	{
		aStats.AddAndResetEntry(STAT_INIT_TIME, m_stateTimes[STATE_INIT]);
		aStats.AddAndResetEntry(STAT_WAITING_TO_CONNECT_TIME, m_stateTimes[STATE_WAITING_TO_CONNECT]);
		aStats.AddAndResetEntry(STAT_WAITING_FOR_CONNECTION_TIME, m_stateTimes[STATE_WAITING_FOR_CONNECTION]);
		aStats.AddAndResetEntry(STAT_CONNECTED_TIME, m_stateTimes[STATE_CONNECTED]);
		aStats.AddAndResetEntry(STAT_DISCONNECTED_TIME, m_stateTimes[STATE_DISCONNECTED]);

		JELLY_ASSERT((size_t)m_state < aOut.size());
		aOut[m_state].Sample((uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_stateTimeStamp).count());
	}

	//-----------------------------------------------------------------------------------------

	void		
	Client::_SetState(
		State									aState)
	{	
		JELLY_ASSERT(m_state != aState);
		JELLY_ASSERT(m_state < NUM_STATES);

		std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();

		uint32_t millisecondsSpentInState = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_stateTimeStamp).count();

		m_stateTimes[m_state].Sample(millisecondsSpentInState);

		m_stateTimeStamp = currentTime;
		m_state = aState;		
	}

}