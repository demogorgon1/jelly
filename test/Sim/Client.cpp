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

	}

	Client::~Client()
	{

	}

	void
	Client::Update(
		Stats&		/*aStats*/)
	{
		switch(m_state)
		{
		case STATE_INIT:
			m_state = STATE_WAITING_TO_CONNECT;
			break;

		case STATE_WAITING_TO_CONNECT:
			{
				GameServer* gameServer = m_network->PickRandomGameServer();

				m_disconnectEvent.Reset();

				m_gameServerConnectRequest = std::make_unique<GameServer::ConnectRequest>(m_id, &m_disconnectEvent);

				gameServer->Connect(m_gameServerConnectRequest.get());

				m_state = STATE_WAITING_FOR_CONNECTION;
			}
			break;

		case STATE_WAITING_FOR_CONNECTION:
			if (m_disconnectEvent.Poll())
			{
				m_state = STATE_DISCONNECTED;
			}
			else if(m_gameServerConnectRequest->m_completed.Poll())
			{
				m_state = STATE_CONNECTED;
			}
			break;

		case STATE_CONNECTED:
			if(m_disconnectEvent.Poll())
				m_state = STATE_DISCONNECTED;
			break;

		case STATE_DISCONNECTED:
			break;
		}
	}

	void		
	Client::UpdateStateCounters(
		std::vector<uint32_t>& aOut)
	{
		JELLY_ASSERT((size_t)m_state < aOut.size());
		aOut[m_state]++;
	}

}