#include <jelly/ErrorUtils.h>

#include "../Config.h"

#include "GameServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	GameServer::GameServer(
		Network*		aNetwork,
		uint32_t		aId)
		: m_network(aNetwork)
		, m_id(aId)
		, m_random(aId)
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
				i.second->m_disconnectEvent->Signal();
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
			//JELLY_ASSERT(aClient->m_pendingConnectRequest != NULL);

			//aClient->m_pendingConnectRequest->m_completed.Signal();
			//aClient->m_pendingConnectRequest = NULL;

			aClient->m_state = Client::STATE_NEED_LOCK;
			aClient->m_timer.SetTimeout(0);
			break;

		case Client::STATE_NEED_LOCK:
			if(aClient->m_timer.HasExpired())
			{
				LockServer::LockNodeType* lockNode = m_network->GetMasterLockServer()->GetNode();
				if(lockNode != NULL)
				{
					aClient->m_lockRequest = std::make_unique<LockServer::LockNodeType::Request>();
					aClient->m_lockRequest->m_key = aClient->m_id;
					aClient->m_lockRequest->m_lock = m_id;
					lockNode->Lock(aClient->m_lockRequest.get());
					
					aClient->m_state = Client::STATE_WAITING_FOR_LOCK;
				}
			}
			break;

		case Client::STATE_WAITING_FOR_LOCK:
			if(aClient->m_lockRequest->m_completed.Poll())
			{
				switch(aClient->m_lockRequest->m_result)
				{
				case RESULT_OK:
					{
						for (uint32_t i = 0; i < 4; i++)
						{
							uint32_t blobNodeId = (aClient->m_lockRequest->m_blobNodeIds & (0xFF << (i * 8))) >> (i * 8);
							if (blobNodeId != 0xFF)
								aClient->m_blobNodeIds.push_back(blobNodeId);
						}

						aClient->m_blobSeq = aClient->m_lockRequest->m_blobSeq;
						aClient->m_nextBlobNodeIdIndex = 0;
						aClient->m_state = Client::STATE_NEED_BLOB;
						aClient->m_timer.SetTimeout(0);
					}
					break;

				case RESULT_ALREADY_LOCKED:
					aClient->m_state = Client::STATE_NEED_LOCK;
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
						aClient->m_getRequest->m_key = aClient->m_id;
						aClient->m_getRequest->m_seq = aClient->m_blobSeq;
						blobNode->Get(aClient->m_getRequest.get());

						aClient->m_state = Client::STATE_WAITING_FOR_BLOB_GET;

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
			if(aClient->m_getRequest->m_completed.Poll())
			{
				switch (aClient->m_getRequest->m_result)
				{
				case RESULT_OK:					
					_OnClientConnected(aClient);
					break;

				default:
					return false;
				}
			}
			break;

		case Client::STATE_CONNECTED:
			if(aClient->m_setTimer.HasExpired())
			{
				aClient->m_setRequest = std::make_unique<BlobServer::BlobNodeType::Request>();
				aClient->m_setRequest->m_key = aClient->m_id;
				aClient->m_setRequest->m_seq = ++aClient->m_blobSeq;				
				aClient->m_playerBlob.ToBlob(aClient->m_setRequest->m_blob);

				uint32_t blobNodeId = m_network->PickRandomBlobNodeId();
				BlobServer::BlobNodeType* blobNode = m_network->GetBlobServer(blobNodeId)->GetNode();

				if(blobNode != NULL)
				{
					blobNode->Set(aClient->m_setRequest.get());

					aClient->m_lastBlobNodeId = blobNodeId;

					aClient->m_state = Client::STATE_WAITING_FOR_BLOB_SET;
				}

				aClient->m_setTimer.SetTimeout(m_network->m_config->m_simSetIntervalMS);
			}
			break;

		case Client::STATE_WAITING_FOR_BLOB_SET:
			if(aClient->m_setRequest->m_completed.Poll())
			{
				switch (aClient->m_setRequest->m_result)
				{
				case RESULT_OK:
					aClient->m_state = Client::STATE_CONNECTED;
					break;

				default:
					return false;
				}
			}
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

		aClient->m_state = Client::STATE_CONNECTED;
	}

	uint32_t	
	GameServer::_GetRandomInInterval(
		uint32_t		aMin,
		uint32_t		aMax)
	{
		std::uniform_int_distribution<uint32_t> distribution(aMin, aMax);
		return distribution(m_random);
	}

}