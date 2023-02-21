#pragma once

#include <random>

#include "Network.h"
#include "PlayerBlob.h"

namespace jelly::Test::Sim
{

	class GameServer
	{
	public:		
		static uint32_t
		GetNumStates()
		{
			return Client::NUM_STATES;
		}

		static uint32_t
		GetStateNumStatsId(
			uint32_t											aState)
		{
			// IMPORTANT: must match Client::State enum
			static const uint32_t IDS[] =
			{
				Stats::ID_GS_C_INIT_NUM,
				Stats::ID_GS_C_NEED_LOCK_NUM,
				Stats::ID_GS_C_WAITING_FOR_LOCK_NUM,
				Stats::ID_GS_C_NEED_BLOB_NUM,
				Stats::ID_GS_C_WAITING_FOR_BLOB_GET_NUM,
				Stats::ID_GS_C_CONNECTED_NUM, 
				Stats::ID_GS_C_WAITING_FOR_BLOB_SET_NUM,
				Stats::ID_GS_C_NEED_UNLOCK_NUM,
				Stats::ID_GS_C_WAITING_FOR_UNLOCK_NUM
			};
			static_assert(sizeof(IDS) == sizeof(uint32_t) * (size_t)Client::NUM_STATES);
			JELLY_ALWAYS_ASSERT(aState < (uint32_t)Client::NUM_STATES);
			return IDS[aState];
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

		void	Update();
		void	UpdateStateStatistics(
					std::vector<uint32_t>&		aStateCounters);
		void	Connect(
					ConnectRequest*				aConnectRequest);
		void	Disconnect(
					uint32_t					aClientId);
		void	TestUngracefulDisconnectRandomClients(
					size_t						aCount);

	private:

		Network*													m_network;
		uint32_t													m_id;
		uint32_t													m_lockId;

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
				, m_stateTimeSampler(NUM_STATES)
				, m_disconnectRequested(false)
				, m_ungracefulDisconnect(false)
			{
				m_stateTimeSampler.DefineState(STATE_INIT, Stats::ID_GS_C_INIT_TIME, Stats::ID_GS_C_INIT_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_NEED_LOCK, Stats::ID_GS_C_NEED_LOCK_TIME, Stats::ID_GS_C_NEED_LOCK_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_WAITING_FOR_LOCK, Stats::ID_GS_C_WAITING_FOR_LOCK_TIME, Stats::ID_GS_C_WAITING_FOR_LOCK_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_NEED_BLOB, Stats::ID_GS_C_NEED_BLOB_TIME, Stats::ID_GS_C_NEED_BLOB_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_WAITING_FOR_BLOB_GET, Stats::ID_GS_C_WAITING_FOR_BLOB_GET_TIME, Stats::ID_GS_C_WAITING_FOR_BLOB_GET_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_CONNECTED, Stats::ID_GS_C_CONNECTED_TIME, Stats::ID_GS_C_CONNECTED_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_WAITING_FOR_BLOB_SET, Stats::ID_GS_C_WAITING_FOR_BLOB_SET_TIME, Stats::ID_GS_C_WAITING_FOR_BLOB_SET_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_NEED_UNLOCK, Stats::ID_GS_C_NEED_UNLOCK_TIME, Stats::ID_GS_C_NEED_UNLOCK_CUR_TIME);
				m_stateTimeSampler.DefineState(STATE_WAITING_FOR_UNLOCK, Stats::ID_GS_C_WAITING_FOR_UNLOCK_TIME, Stats::ID_GS_C_WAITING_FOR_UNLOCK_CUR_TIME);
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
				STATE_NEED_UNLOCK,
				STATE_WAITING_FOR_UNLOCK,

				NUM_STATES
			};

			State													m_state;
			StateTimeSampler										m_stateTimeSampler;

			ConnectRequest*											m_pendingConnectRequest;
			CompletionEvent*										m_disconnectEvent;
			bool													m_disconnectRequested;
			bool													m_ungracefulDisconnect;
			std::unique_ptr<LockServer::LockNodeType::Request>		m_lockRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>		m_getRequest;
			std::unique_ptr<BlobServer::BlobNodeType::Request>		m_setRequest;
			std::unique_ptr<LockServer::LockNodeType::Request>		m_unlockRequest;

			std::vector<uint32_t>									m_blobNodeIds;
			size_t													m_nextBlobNodeIdIndex;
			uint32_t												m_lastBlobNodeId;
			uint32_t												m_blobSeq;

			Timer													m_timer;
			Timer													m_setTimer;

			PlayerBlob												m_playerBlob;
		};

		std::unordered_map<uint32_t, Client*>						m_clients;
		uint32_t													m_updateId;
		std::mt19937												m_random;

		std::mutex													m_connectRequestsLock;
		std::vector<ConnectRequest*>								m_connectRequests;

		std::mutex													m_disconnectRequestsLock;
		std::vector<uint32_t>										m_disconnectRequests;

		std::mutex													m_testEventLock;
		std::optional<size_t>										m_testUngracefulDisconnectRandomClientCount;

		void		_ProcessTestEvents();
		void		_ProcessRequests();		
		void		_UpdateClients();
		bool		_UpdateClient(
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