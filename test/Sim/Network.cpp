#include "../Config.h"

#include "BlobServer.h"
#include "Client.h"
#include "GameServer.h"
#include "LockServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	Network::Network(
		const char*		aWorkingDirectory,
		const Config*	aConfig)
		: m_host(aWorkingDirectory, DefaultHost::COMPRESSION_MODE_ZSTD)
		, m_config(aConfig)
	{
		for(uint32_t i = 0; i < aConfig->m_simNumClients; i++)
			m_clients.push_back(new Client(this, i));

		for (uint32_t i = 0; i < aConfig->m_simNumGameServers; i++)
			m_gameServers.push_back(new GameServer(this, i));

		uint32_t nodeId = 0;

		for (uint32_t i = 0; i < aConfig->m_simNumLockServers; i++)
			m_lockServers.push_back(new LockServer(this, nodeId++));

		for (uint32_t i = 0; i < aConfig->m_simNumBlobServers; i++)
			m_blobServers.push_back(new BlobServer(this, nodeId++));

	}
	
	Network::~Network()
	{	
		for(Client* t : m_clients)
			delete t;

		for (GameServer* t : m_gameServers)
			delete t;

		for (LockServer* t : m_lockServers)
			delete t;

		for (BlobServer* t : m_blobServers)
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

}