#pragma once

#include <mutex>
#include <random>
#include <unordered_map>

#include <stdint.h>

#include <jelly/CompletionEvent.h>

#include "Network.h"
#include "PlayerBlob.h"
#include "Timer.h"

namespace jelly::Test::Sim
{

	class GameServer
	{
	public:		
		enum Stat : uint32_t
		{
			STAT_NUM_CLIENTS,
			STAT_SET_REQUESTS,
			STAT_GET_REQUESTS,
			STAT_LOCK_REQUESTS,
			STAT_UNLOCK_REQUESTS,

			NUM_STATS
		};

		static void
		InitStats(
			Stats&			aStats)
		{
			aStats.Init(NUM_STATS);
		}

		static void
		PrintStats(
			const Stats&					aStats,
			const std::vector<uint32_t>&	aStateCounters)
		{
			aStats.Print(Stats::TYPE_SAMPLE, STAT_NUM_CLIENTS, "NUM_CLIENTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_SET_REQUESTS, "SET_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_GET_REQUESTS, "GET_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_LOCK_REQUESTS, "LOCK_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_UNLOCK_REQUESTS, "UNLOCK_REQUESTS");

			Stats::PrintStateCounter("INIT", Client::STATE_INIT, aStateCounters);
			Stats::PrintStateCounter("NEED_LOCK", Client::STATE_NEED_LOCK, aStateCounters);
			Stats::PrintStateCounter("WAITING_FOR_LOCK", Client::STATE_WAITING_FOR_LOCK, aStateCounters);
			Stats::PrintStateCounter("NEED_BLOB", Client::STATE_NEED_BLOB, aStateCounters);
			Stats::PrintStateCounter("WAITING_FOR_BLOB_GET", Client::STATE_WAITING_FOR_BLOB_GET, aStateCounters);
			Stats::PrintStateCounter("CONNECTED", Client::STATE_CONNECTED, aStateCounters);
			Stats::PrintStateCounter("WAITING_FOR_BLOB_SET", Client::STATE_WAITING_FOR_BLOB_SET, aStateCounters);
		}

		static uint32_t
		GetNumStates()
		{
			return Client::NUM_STATES;
		}

		struct ConnectRequest
		{	
			ConnectRequest(
				uint32_t			aClientId = 0,
				CompletionEvent*	aDisconnectEvent = NULL)
				: m_clientId(aClientId)
				, m_disconnectEvent(aDisconnectEvent)
			{

			}

			uint32_t												m_clientId;
			CompletionEvent											m_completed;
			CompletionEvent*										m_disconnectEvent;
		};

				GameServer(
					Network*				aNetwork,
					uint32_t				aId);
				~GameServer();

		void	Update(
					IHost*					aHost,
					Stats&					aStats);
		void	UpdateStateCounters(
					std::vector<uint32_t>&	aOut);
		void	Connect(
					ConnectRequest*			aConnectRequest);

	private:

		Network*													m_network;
		uint32_t													m_id;

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
			uint32_t												m_id;

			enum State : uint32_t
			{
				STATE_INIT,
				STATE_NEED_LOCK,
				STATE_WAITING_FOR_LOCK,
				STATE_NEED_BLOB,
				STATE_WAITING_FOR_BLOB_GET,
				STATE_CONNECTED,
				STATE_WAITING_FOR_BLOB_SET,

				NUM_STATES
			};

			State													m_state;
			ConnectRequest*											m_pendingConnectRequest;
			CompletionEvent*										m_disconnectEvent;
			std::unique_ptr<LockServer::LockNodeType::Request>		m_lockRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>		m_getRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>		m_setRequest;

			std::vector<uint32_t>									m_blobNodeIds;
			size_t													m_nextBlobNodeIdIndex;
			uint32_t												m_lastBlobNodeId;
			uint32_t												m_blobSeq;

			Timer													m_timer;
			Timer													m_setTimer;

			PlayerBlob												m_playerBlob;
		};

		std::unordered_map<uint32_t, Client*>						m_clients;

		std::mt19937												m_random;

		std::mutex													m_connectRequestsLock;
		std::vector<ConnectRequest*>								m_connectRequests;

		void		_ProcessRequests();
		void		_UpdateClients(
						Stats&			aStats);
		bool		_UpdateClient(
						Stats&			aStats,
						Client*			aClient);
		void		_OnClientConnected(
						Client*			aClient);
		uint32_t	_GetRandomInInterval(
						uint32_t		aMin,
						uint32_t		aMax);
	};
	
}