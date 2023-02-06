#pragma once

#include <random>

#include "BlobServer.h"
#include "LockServer.h"
#include "SharedWorkQueue.h"

namespace jelly::Test
{

	struct Config;

	namespace Sim
	{

		class Client;
		class GameServer;
		class SharedWorkerThread;

		class Network
		{
		public:
										Network(
											const char*		aWorkingDirectory,
											const Config*	aConfig);
										~Network();

			GameServer*					PickRandomGameServer();
			LockServer::LockServerType* GetMasterLockServer();
			BlobServer::BlobServerType*	GetBlobServer(
											uint32_t		aBlobNodeId);
			uint32_t					PickRandomBlobNodeId();

			// Public data
			const Config*										m_config;

			std::vector<Client*>								m_clients;
			std::vector<GameServer*>							m_gameServers;
			std::vector<LockServer::LockServerType*>			m_lockServers;
			std::vector<BlobServer::BlobServerType*>			m_blobServers;

			DefaultHost											m_host;
			uint32_t											m_firstBlobNodeId;

			SharedWorkQueue										m_sharedWorkQueue;
			std::vector<std::unique_ptr<SharedWorkerThread>>	m_sharedWorkerThreads;

			std::mutex											m_randomLock;
			std::mt19937										m_random;
		};

	}

}