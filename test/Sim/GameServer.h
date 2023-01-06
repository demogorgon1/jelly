#pragma once

#include <mutex>
#include <unordered_map>

#include <stdint.h>

#include <jelly/CompletionEvent.h>

namespace jelly::Test::Sim
{

	class Network;

	class GameServer
	{
	public:		
		struct ConnectRequest
		{	
			ConnectRequest(
				uint32_t			aClientId = 0,
				CompletionEvent*	aDisconnectEvent = NULL)
				: m_clientId(aClientId)
				, m_disconnectEvent(aDisconnectEvent)
			{

			}

			uint32_t							m_clientId;
			CompletionEvent						m_completed;
			CompletionEvent*					m_disconnectEvent;
		};

				GameServer(
					Network*		aNetwork,
					uint32_t		aId);
				~GameServer();

		void	Update();
		void	Connect(
					ConnectRequest*	aConnectRequest);

	private:

		Network*								m_network;
		uint32_t								m_id;

		struct Client
		{
			Client(
				uint32_t			aId,
				ConnectRequest*		aPendingConnectRequest,
				CompletionEvent*	aDisconnectEvent)
				: m_id(aId)
				, m_state(STATE_INIT)
				, m_pendingConnectRequest(aPendingConnectRequest)
				, m_disconnectEvent(aDisconnectEvent)
			{

			}

			// Public data
			uint32_t							m_id;

			enum State
			{
				STATE_INIT,
				STATE_CONNECTED
			};

			State								m_state;
			ConnectRequest*						m_pendingConnectRequest;
			CompletionEvent*					m_disconnectEvent;
		};

		std::unordered_map<uint32_t, Client*>	m_clients;

		std::mutex								m_connectRequestsLock;
		std::vector<ConnectRequest*>			m_connectRequests;

		void	_ProcessRequests();
		void	_UpdateClients();
		bool	_UpdateClient(
					Client*			aClient);
	};
	
}