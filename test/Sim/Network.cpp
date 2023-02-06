#include <jelly/API.h>

#include "../Config.h"

#include "Client.h"
#include "GameServer.h"
#include "Network.h"
#include "SharedWorkerThread.h"
#include "Stats.h"

namespace jelly::Test::Sim
{

	Network::Network(
		const char*		aWorkingDirectory,
		const Config*	aConfig)
		: m_host(
			aWorkingDirectory, 
			"simtest", 
			Compression::ID_ZSTD, 
			aConfig->m_simBufferCompressionLevel, 
			Stats::GetExtraApplicationStats(),
			Stats::GetExtraApplicationStatsCount())
		, m_config(aConfig)
		, m_clientLimit(aConfig->m_simStartClientLimit)
	{
		for (uint32_t i = 0; i < aConfig->m_simNumSharedWorkerThreads; i++)
			m_sharedWorkerThreads.push_back(std::make_unique<SharedWorkerThread>(&m_sharedWorkQueue));

		for(uint32_t i = 0; i < aConfig->m_simNumClients; i++)
			m_clients.push_back(new Client(this, i));

		for (uint32_t i = 0; i < aConfig->m_simNumGameServers; i++)
			m_gameServers.push_back(new GameServer(this, i));

		uint32_t nodeId = 0;

		for (uint32_t i = 0; i < aConfig->m_simNumLockServers; i++)
			m_lockServers.push_back(new LockServer::LockServerType(this, &m_host, nodeId++, aConfig->m_simLockNodeConfig));

		m_firstBlobNodeId = nodeId;

		for (uint32_t i = 0; i < aConfig->m_simNumBlobServers; i++)
			m_blobServers.push_back(new BlobServer::BlobServerType(this, &m_host, nodeId++, aConfig->m_simBlobNodeConfig));
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

	void						
	Network::UpdateStats()
	{	
		m_host.PollSystemStats();

		m_host.GetStats()->Emit(Stats::ID_CLIENT_LIMIT, (uint32_t)m_clientLimit);

		m_host.GetStats()->Update();
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