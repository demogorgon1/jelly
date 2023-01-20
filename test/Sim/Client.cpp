#include <jelly/API.h>

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
		, m_stateTimeSampler(NUM_STATES)
	{
		m_stateTimeSampler.DefineState(STATE_INIT, Stats::ID_C_INIT_TIME, Stats::ID_C_INIT_CUR_TIME);
		m_stateTimeSampler.DefineState(STATE_WAITING_TO_CONNECT, Stats::ID_C_WAITING_TO_CONNECT_TIME, Stats::ID_C_WAITING_TO_CONNECT_CUR_TIME);
		m_stateTimeSampler.DefineState(STATE_WAITING_FOR_CONNECTION, Stats::ID_C_WAITING_FOR_CONNECTION_TIME, Stats::ID_C_WAITING_FOR_CONNECTION_CUR_TIME);
		m_stateTimeSampler.DefineState(STATE_CONNECTED, Stats::ID_C_CONNECTED_TIME, Stats::ID_C_CONNECTED_CUR_TIME);
		m_stateTimeSampler.DefineState(STATE_DISCONNECTED, Stats::ID_C_DISCONNECTED_TIME, Stats::ID_C_DISCONNECTED_CUR_TIME);
	}

	Client::~Client()
	{

	}

	void
	Client::Update()
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
	Client::UpdateStateStatistics(
		std::vector<uint32_t>&					aStateCounters)
	{
		JELLY_ASSERT((size_t)m_state < aStateCounters.size());

		aStateCounters[m_state]++;

		m_stateTimeSampler.EmitCurrentStateTime(m_network->m_host.GetStats(), std::chrono::steady_clock::now());
	}

	//-----------------------------------------------------------------------------------------

	void		
	Client::_SetState(
		State									aState)
	{	
		JELLY_ASSERT(m_state != aState);
		m_stateTimeSampler.ChangeState(m_network->m_host.GetStats(), aState);
		m_state = aState;		
	}

}