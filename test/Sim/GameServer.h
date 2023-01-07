#pragma once

#include <mutex>
#include <random>
#include <unordered_map>

#include <stdint.h>

#include <jelly/CompletionEvent.h>

#include "BlobServer.h"
#include "LockServer.h"
#include "PlayerBlob.h"
#include "Timer.h"

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

			uint32_t											m_clientId;
			CompletionEvent										m_completed;
			CompletionEvent*									m_disconnectEvent;
		};

				GameServer(
					Network*		aNetwork,
					uint32_t		aId);
				~GameServer();

		void	Update();
		void	Connect(
					ConnectRequest*	aConnectRequest);

	private:

		Network*												m_network;
		uint32_t												m_id;

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
				, m_blobSeq(0)
			{

			}

			// Public data
			uint32_t											m_id;

			enum State
			{
				STATE_INIT,
				STATE_NEED_LOCK,
				STATE_WAITING_FOR_LOCK,
				STATE_NEED_BLOB,
				STATE_WAITING_FOR_BLOB_GET,
				STATE_CONNECTED,
				STATE_WAITING_FOR_BLOB_SET
			};

			State												m_state;
			ConnectRequest*										m_pendingConnectRequest;
			CompletionEvent*									m_disconnectEvent;
			std::unique_ptr<LockServer::LockNodeType::Request>	m_lockRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>	m_getRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>	m_setRequest;

			std::vector<uint32_t>								m_blobNodeIds;
			size_t												m_nextBlobNodeIdIndex;
			uint32_t											m_lastBlobNodeId;
			uint32_t											m_blobSeq;

			Timer												m_timer;
			Timer												m_setTimer;

			PlayerBlob											m_playerBlob;
		};

		std::unordered_map<uint32_t, Client*>					m_clients;

		std::mt19937											m_random;

		std::mutex												m_connectRequestsLock;
		std::vector<ConnectRequest*>							m_connectRequests;

		void		_ProcessRequests();
		void		_UpdateClients();
		bool		_UpdateClient(
						Client*			aClient);
		void		_OnClientConnected(
						Client*			aClient);
		uint32_t	_GetRandomInInterval(
						uint32_t		aMin,
						uint32_t		aMax);
	};
	
}