#include <jelly/ErrorUtils.h>

#include "GameServer.h"

namespace jelly::Test::Sim
{

	GameServer::GameServer(
		Network*		aNetwork,
		uint32_t		aId)
		: m_network(aNetwork)
		, m_id(aId)
	{

	}

	GameServer::~GameServer()
	{
		for(std::pair<uint32_t, Client*> i : m_clients)
			delete i.second;
	}

	void
	GameServer::Update()
	{
		_ProcessRequests();
		_UpdateClients();
	}

	void	
	GameServer::Connect(
		ConnectRequest*		aConnectRequest)
	{
		std::lock_guard lock(m_connectRequestsLock);
		m_connectRequests.push_back(aConnectRequest);
	}

	//-------------------------------------------------------------------------------------------

	void	
	GameServer::_ProcessRequests()
	{
		std::vector<ConnectRequest*> connectRequests;

		{
			std::lock_guard lock(m_connectRequestsLock);
			connectRequests = m_connectRequests;
			m_connectRequests.clear();
		}

		for(ConnectRequest* connectRequest : connectRequests)
		{
			std::unordered_map<uint32_t, Client*>::iterator i = m_clients.find(connectRequest->m_clientId);
			JELLY_ASSERT(i == m_clients.end());
			m_clients.insert(std::make_pair(connectRequest->m_clientId, new Client(connectRequest->m_clientId, connectRequest, connectRequest->m_disconnectEvent)));
		}
	}
	
	void	
	GameServer::_UpdateClients()
	{
		for (std::pair<uint32_t, Client*> i : m_clients)
		{
			if (!_UpdateClient(i.second))
			{
				delete i.second;
				m_clients.erase(i.first);
			}
		}
	}

	bool	
	GameServer::_UpdateClient(
		Client*			aClient)
	{
		switch(aClient->m_state)
		{
		case Client::STATE_INIT:
			JELLY_ASSERT(aClient->m_pendingConnectRequest != NULL);
			
			aClient->m_pendingConnectRequest->m_completed.Signal();
			aClient->m_pendingConnectRequest = NULL;

			aClient->m_state = Client::STATE_CONNECTED;
			break;

		case Client::STATE_CONNECTED:
			break;
		}

		return true;
	}

}