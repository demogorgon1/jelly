#pragma once

#include <stdint.h>

#include "GameServer.h"

namespace jelly::Test::Sim
{

	class Network;

	class Client
	{
	public:
				Client(
					Network*		aNetwork,
					uint32_t		aId);
				~Client();

		void	Update();

	private:

		Network*									m_network;
		uint32_t									m_id;

		enum State
		{
			STATE_INIT,
			STATE_WAITING_TO_CONNECT,
			STATE_WAITING_FOR_CONNECTION,
			STATE_CONNECTED,
			STATE_DISCONNECTED
		};

		State										m_state;

		std::unique_ptr<GameServer::ConnectRequest>	m_gameServerConnectRequest;

		CompletionEvent								m_disconnectEvent;
	};
	
}