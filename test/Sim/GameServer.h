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
			STAT_INIT_TIME,
			STAT_NEED_LOCK_TIME,
			STAT_WAITING_FOR_LOCK_TIME,
			STAT_NEED_BLOB_TIME,
			STAT_WAITING_FOR_BLOB_GET_TIME,
			STAT_CONNECTED_TIME,
			STAT_WAITING_FOR_BLOB_SET_TIME,

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
			const Stats&						aStats,
			const std::vector<Stats::Entry>&	aStateInfo)
		{
			aStats.Print(Stats::TYPE_SAMPLE, STAT_NUM_CLIENTS, "NUM_CLIENTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_SET_REQUESTS, "SET_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_GET_REQUESTS, "GET_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_LOCK_REQUESTS, "LOCK_REQUESTS");
			aStats.Print(Stats::TYPE_COUNTER, STAT_UNLOCK_REQUESTS, "UNLOCK_REQUESTS");

			Stats::PrintStateInfo("INIT", Client::STATE_INIT, aStateInfo, aStats, STAT_INIT_TIME);
			Stats::PrintStateInfo("NEED_LOCK", Client::STATE_NEED_LOCK, aStateInfo, aStats, STAT_NEED_LOCK_TIME);
			Stats::PrintStateInfo("WAITING_FOR_LOCK", Client::STATE_WAITING_FOR_LOCK, aStateInfo, aStats, STAT_WAITING_FOR_LOCK_TIME);
			Stats::PrintStateInfo("NEED_BLOB", Client::STATE_NEED_BLOB, aStateInfo, aStats, STAT_NEED_BLOB_TIME);
			Stats::PrintStateInfo("WAITING_FOR_BLOB_GET", Client::STATE_WAITING_FOR_BLOB_GET, aStateInfo, aStats, STAT_WAITING_FOR_BLOB_GET_TIME);
			Stats::PrintStateInfo("CONNECTED", Client::STATE_CONNECTED, aStateInfo, aStats, STAT_CONNECTED_TIME);
			Stats::PrintStateInfo("WAITING_FOR_BLOB_SET", Client::STATE_WAITING_FOR_BLOB_SET, aStateInfo, aStats, STAT_WAITING_FOR_BLOB_SET_TIME);
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
					Network*					aNetwork,
					uint32_t					aId);
				~GameServer();

		void	Update(
					IHost*						aHost,
					Stats&						aStats);
		void	UpdateStateInfo(
					Stats&						aStats,
					std::vector<Stats::Entry>&	aOut);
		void	Connect(
					ConnectRequest*				aConnectRequest);

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
				m_stateTimeStamp = std::chrono::steady_clock::now();
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
			std::chrono::time_point<std::chrono::steady_clock>		m_stateTimeStamp;

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

		Stats::Entry												m_clientStateTimes[Client::NUM_STATES];

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
		void		_SetClientState(
						Client*			aClient,
						Client::State	aState);
	};
	
}