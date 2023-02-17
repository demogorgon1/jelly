#include <jelly/API.h>

#include "../Config.h"

#include "GameServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	GameServer::GameServer(
		Network*				aNetwork,
		uint32_t				aId)
		: m_network(aNetwork)
		, m_id(aId)
		, m_lockId(aId + 1)
		, m_random(aId)
		, m_updateId(0)
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
		ScopedTimeSampler timer(m_network->m_host.GetStats(), Stats::ID_GS_UPDATE_TIME);

		if(m_updateId++ % 100)
			_ProcessTestEvents();

		_ProcessRequests();
		_UpdateClients();

		m_network->m_host.GetStats()->Emit(Stats::ID_GS_NUM_CLIENTS, m_clients.size());
	}

	void	
	GameServer::UpdateStateStatistics(
		std::vector<uint32_t>&	aStateCounters)
	{
		std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();
		
		for (std::pair<uint32_t, Client*> i : m_clients)
		{
			JELLY_ASSERT((size_t)i.second->m_state < aStateCounters.size());

			aStateCounters[i.second->m_state]++;

			i.second->m_stateTimeSampler.EmitCurrentStateTime(m_network->m_host.GetStats(), currentTime);
		}
	}

	void	
	GameServer::Connect(
		ConnectRequest*			aConnectRequest)
	{
		std::lock_guard lock(m_connectRequestsLock);
		m_connectRequests.push_back(aConnectRequest);
	}

	void	
	GameServer::Disconnect(
		uint32_t				aClientId)
	{
		std::lock_guard lock(m_disconnectRequestsLock);
		m_disconnectRequests.push_back(aClientId);
	}

	void	
	GameServer::TestUngracefulDisconnectRandomClients(
		size_t					aCount)
	{
		std::lock_guard lock(m_testEventLock);
		m_testUngracefulDisconnectRandomClientCount = aCount;
	}

	//-------------------------------------------------------------------------------------------

	void
	GameServer::_ProcessTestEvents()
	{
		size_t randomClientsToDisconnect = 0;

		{
			std::lock_guard lock(m_testEventLock);
			if(m_testUngracefulDisconnectRandomClientCount.has_value())
			{
				randomClientsToDisconnect = m_testUngracefulDisconnectRandomClientCount.value();
				m_testUngracefulDisconnectRandomClientCount.reset();
			}
		}

		std::unordered_map<uint32_t, Client*>::iterator it = m_clients.begin();
		for(size_t i = 0; it != m_clients.end() && i < randomClientsToDisconnect; i++)
		{
			it->second->m_ungracefulDisconnect = true;
			it++;
		}
	}

	void	
	GameServer::_ProcessRequests()
	{
		// Connect requests
		{
			std::vector<ConnectRequest*> connectRequests;

			{
				std::lock_guard lock(m_connectRequestsLock);
				connectRequests = m_connectRequests;
				m_connectRequests.clear();
			}

			for (ConnectRequest* connectRequest : connectRequests)
			{
				std::unordered_map<uint32_t, Client*>::iterator i = m_clients.find(connectRequest->m_clientId);
				JELLY_ASSERT(i == m_clients.end());
				m_clients.insert(std::make_pair(connectRequest->m_clientId, new Client(connectRequest->m_clientId, connectRequest, connectRequest->m_disconnectEvent)));
			}
		}

		// Disconnect requests
		{
			std::vector<uint32_t> disconnectRequests;

			{
				std::lock_guard lock(m_disconnectRequestsLock);
				disconnectRequests = m_disconnectRequests;
				m_disconnectRequests.clear();
			}

			for(uint32_t clientId : disconnectRequests)
			{
				std::unordered_map<uint32_t, Client*>::iterator i = m_clients.find(clientId);
				JELLY_ASSERT(i != m_clients.end());
				JELLY_ASSERT(!i->second->m_disconnectRequested);
				i->second->m_disconnectRequested = true;								
			}
		}
	}
	
	void	
	GameServer::_UpdateClients()
	{
		std::vector<uint32_t> deletedClientIds;

		for (std::pair<uint32_t, Client*> i : m_clients)
		{
			if (!_UpdateClient(i.second))
			{
				i.second->m_disconnectEvent->Signal();
				delete i.second;
				deletedClientIds.push_back(i.first);
			}
		}

		for(uint32_t deletedClientId : deletedClientIds)
			m_clients.erase(deletedClientId);
	}

	bool	
	GameServer::_UpdateClient(
		Client*			aClient)
	{
		switch(aClient->m_state)
		{
		case Client::STATE_INIT:
			_SetClientState(aClient, Client::STATE_NEED_LOCK);
			aClient->m_timer.SetTimeout(0);
			break;

		case Client::STATE_NEED_LOCK:
			if(aClient->m_timer.HasExpired())
			{
				LockServer::LockNodeType* lockNode = m_network->GetMasterLockServer()->GetNode();
				if(lockNode != NULL)
				{
					aClient->m_lockRequest = std::make_unique<LockServer::LockNodeType::Request>();
					aClient->m_lockRequest->SetKey(aClient->m_id);
					aClient->m_lockRequest->SetLock(m_lockId);
					lockNode->Lock(aClient->m_lockRequest.get());
					
					_SetClientState(aClient, Client::STATE_WAITING_FOR_LOCK);
				}
			}
			break;

		case Client::STATE_WAITING_FOR_LOCK:
			if(aClient->m_lockRequest->IsCompleted())
			{
				switch(aClient->m_lockRequest->GetResult())
				{
				case RESULT_OK:
					{
						const LockServer::LockMetaDataType& meta = aClient->m_lockRequest->GetMeta();

						for (uint32_t i = 0; i < meta.m_blobNodeIdCount; i++)
							aClient->m_blobNodeIds.push_back(meta.m_blobNodeIds[i]);

						aClient->m_blobSeq = meta.m_blobSeq;

						aClient->m_nextBlobNodeIdIndex = 0;
						_SetClientState(aClient, Client::STATE_NEED_BLOB);
						aClient->m_timer.SetTimeout(0);
					}
					break;

				case RESULT_ALREADY_LOCKED:
					_SetClientState(aClient, Client::STATE_NEED_LOCK);
					aClient->m_timer.SetTimeout(1000);
					break;

				default:
					return false;
				}
			}
			break;

		case Client::STATE_NEED_BLOB:
			if(aClient->m_blobSeq == UINT32_MAX)
			{
				// This is a new client, doesn't have a blob
				aClient->m_playerBlob.GenerateSomeData(m_random);

				_OnClientConnected(aClient);
			}				
			else if (aClient->m_nextBlobNodeIdIndex < aClient->m_blobNodeIds.size())
			{
				if(aClient->m_timer.HasExpired())
				{
					// Try to get blob from this blob node
					uint32_t blobNodeId = aClient->m_blobNodeIds[aClient->m_nextBlobNodeIdIndex];
					BlobServer::BlobNodeType* blobNode = m_network->GetBlobServer(blobNodeId)->GetNode();

					if(blobNode != NULL)
					{
						aClient->m_getRequest = std::make_unique<BlobServer::BlobNodeType::Request>();
						aClient->m_getRequest->SetKey(aClient->m_id);
						aClient->m_getRequest->SetSeq(aClient->m_blobSeq);
						blobNode->Get(aClient->m_getRequest.get());

						_SetClientState(aClient, Client::STATE_WAITING_FOR_BLOB_GET);

						aClient->m_nextBlobNodeIdIndex++;
					}				
				}
			}
			else 
			{
				// We tried to get the blob from all the specified blob nodes - try to recover latest blob we can find
				JELLY_ASSERT(false);
			}
			break;

		case Client::STATE_WAITING_FOR_BLOB_GET:
			if(aClient->m_getRequest->IsCompleted())
			{
				switch (aClient->m_getRequest->GetResult())
				{
				case RESULT_OK:					
					aClient->m_playerBlob.FromBlob(aClient->m_getRequest->GetBlob());

					_OnClientConnected(aClient);
					break;

				default:
					return false;
				}
			}
			break;

		case Client::STATE_CONNECTED:
			if(aClient->m_setTimer.HasExpired() || aClient->m_disconnectRequested || aClient->m_ungracefulDisconnect)
			{
				aClient->m_setRequest = std::make_unique<BlobServer::BlobNodeType::Request>();
				aClient->m_setRequest->SetKey(aClient->m_id);
				aClient->m_setRequest->SetSeq(++aClient->m_blobSeq);
				aClient->m_setRequest->SetBlob(aClient->m_playerBlob.AsBlob());

				uint32_t blobNodeId = m_network->PickRandomBlobNodeId();
				BlobServer::BlobNodeType* blobNode = m_network->GetBlobServer(blobNodeId)->GetNode();

				if(blobNode != NULL)
				{
					blobNode->Set(aClient->m_setRequest.get());

					aClient->m_lastBlobNodeId = blobNodeId;

					_SetClientState(aClient, Client::STATE_WAITING_FOR_BLOB_SET);
				}

				aClient->m_setTimer.SetTimeout(m_network->m_config->m_simSetIntervalMS);
			}
			break;

		case Client::STATE_WAITING_FOR_BLOB_SET:
			if(aClient->m_setRequest->IsCompleted())
			{
				switch (aClient->m_setRequest->GetResult())
				{
				case RESULT_OK:
					if(aClient->m_ungracefulDisconnect)
						return false;
					else if(aClient->m_disconnectRequested)
						_SetClientState(aClient, Client::STATE_NEED_UNLOCK);
					else
						_SetClientState(aClient, Client::STATE_CONNECTED);
					break;

				default:
					return false;
				}
			}
			break;

		case Client::STATE_NEED_UNLOCK:
			if (aClient->m_timer.HasExpired())
			{
				LockServer::LockNodeType* lockNode = m_network->GetMasterLockServer()->GetNode();
				if (lockNode == NULL)
				{
					aClient->m_timer.SetTimeout(1000);
				}
				else
				{
					aClient->m_unlockRequest = std::make_unique<LockServer::LockNodeType::Request>();
					aClient->m_unlockRequest->SetKey(aClient->m_id);
					aClient->m_unlockRequest->SetLock(m_lockId);
					aClient->m_unlockRequest->SetMeta(LockServer::LockMetaDataType(aClient->m_blobSeq, { aClient->m_lastBlobNodeId }));
					lockNode->Unlock(aClient->m_unlockRequest.get());

					_SetClientState(aClient, Client::STATE_WAITING_FOR_UNLOCK);
				}					
			}
			break;

		case Client::STATE_WAITING_FOR_UNLOCK:	
			if(aClient->m_unlockRequest->IsCompleted())
			{
				switch (aClient->m_unlockRequest->GetResult())
				{
				case RESULT_OK:
					return false;

				default:
					JELLY_ASSERT(false);
				}
			}
			break;

		default:
			JELLY_ASSERT(false);
			break;
		}

		return true;
	}

	void	
	GameServer::_OnClientConnected(
		Client*			aClient)
	{
		JELLY_ASSERT(aClient->m_pendingConnectRequest != NULL);
		aClient->m_pendingConnectRequest->m_completed.Signal();
		aClient->m_pendingConnectRequest = NULL;

		// Randomize first set to get better distribution
		aClient->m_setTimer.SetTimeout(_GetRandomInInterval(0, m_network->m_config->m_simSetIntervalMS * 2));

		_SetClientState(aClient, Client::STATE_CONNECTED);
	}

	uint32_t	
	GameServer::_GetRandomInInterval(
		uint32_t		aMin,
		uint32_t		aMax)
	{
		std::uniform_int_distribution<uint32_t> distribution(aMin, aMax);
		return distribution(m_random);
	}

	void		
	GameServer::_SetClientState(
		Client*			aClient,
		Client::State	aState)
	{
		JELLY_ASSERT(aClient->m_state != aState);
		aClient->m_stateTimeSampler.ChangeState(m_network->m_host.GetStats(), aState);
		aClient->m_state = aState;
	}

}