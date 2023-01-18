#include <jelly/API.h>

#include "../Config.h"

#include "Client.h"
#include "GameServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	Network::Network(
		const char*		aWorkingDirectory,
		const Config*	aConfig)
		: m_host(aWorkingDirectory, "simtest", Compression::ID_ZSTD)
		, m_config(aConfig)
	{
		for(uint32_t i = 0; i < aConfig->m_simNumClients; i++)
			m_clients.push_back(new Client(this, i));

		for (uint32_t i = 0; i < aConfig->m_simNumGameServers; i++)
			m_gameServers.push_back(new GameServer(this, i));

		uint32_t nodeId = 0;

		for (uint32_t i = 0; i < aConfig->m_simNumLockServers; i++)
			m_lockServers.push_back(new LockServer::LockServerType(this, nodeId++));

		m_firstBlobNodeId = nodeId;

		for (uint32_t i = 0; i < aConfig->m_simNumBlobServers; i++)
			m_blobServers.push_back(new BlobServer::BlobServerType(this, nodeId++));

	}
	
	Network::~Network()
	{	
		for(Client* t : m_clients)
			delete t;

		for (GameServer* t : m_gameServers)
			delete t;

		for (LockServer::LockServerType* t : m_lockServers)
			delete t;

		for (BlobServer::BlobServerType* t : m_blobServers)
			delete t;
	}

	GameServer* 
	Network::PickRandomGameServer()
	{
		GameServer* t;
		std::uniform_int_distribution<size_t> distribution(0, m_gameServers.size() - 1);

		{
			std::lock_guard lock(m_randomLock);
			t = m_gameServers[distribution(m_random)];
		}

		return t;
	}

	LockServer::LockServerType*
	Network::GetMasterLockServer()
	{
		JELLY_ASSERT(m_lockServers.size() > 0);
		return m_lockServers[0];
	}

	BlobServer::BlobServerType*
	Network::GetBlobServer(
		uint32_t		aBlobNodeId)
	{
		JELLY_ASSERT(aBlobNodeId >= m_firstBlobNodeId);
		size_t i = (size_t)(aBlobNodeId - m_firstBlobNodeId);
		JELLY_ASSERT(i < m_blobServers.size());
		return m_blobServers[i];
	}

	uint32_t	
	Network::PickRandomBlobNodeId()
	{
		uint32_t blobNodeId;
		std::uniform_int_distribution<size_t> distribution(0, m_blobServers.size() - 1);
		
		{
			std::lock_guard lock(m_randomLock);
			blobNodeId = (uint32_t)distribution(m_random) + m_firstBlobNodeId;
		}

		return blobNodeId;
	}

}