#include <memory>
#include <thread>

#include "../Config.h"

#include "Client.h"
#include "BlobServer.h"
#include "GameServer.h"
#include "LockServer.h"
#include "Network.h"
#include "Stats.h"
#include "Timer.h"

namespace jelly::Test::Sim
{
	
	namespace
	{

		template <typename _T>
		class Thread
		{
		public:
			Thread(
				IHost*		aHost)
				: m_stopEvent(false)
				, m_host(aHost)
			{				
				_T::InitStats(m_readStats);
				_T::InitStats(m_writeStats);

				m_stateCounters.resize(_T::GetNumStates());

				m_updateTimer.SetTimeout(1000);
			}

			~Thread()
			{
				BlockingStop();
			}

			void
			Add(
				_T*			aObject)
			{
				JELLY_ASSERT(!m_thread);
				m_objects.push_back(aObject);
			}

			void
			Start()
			{
				JELLY_ASSERT(!m_thread);
				m_stopEvent = false;
				m_thread = std::make_unique<std::thread>(_ThreadMain, this);
			}

			void
			BlockingStop()
			{
				if (m_thread)
				{
					m_stopEvent = true;
					m_thread->join();
					m_thread.reset();
				}
			}

			void
			GetStats(
				Stats&					aOutStats,
				std::vector<uint32_t>&	aOutStateCounters)
			{
				std::lock_guard lock(m_readStatsLock);

				aOutStats = m_readStats;
				m_readStats.Reset();

				JELLY_ASSERT(m_stateCounters.size() == aOutStateCounters.size());
				for(size_t i = 0; i < m_stateCounters.size(); i++)
					aOutStateCounters[i] += m_stateCounters[i];
			}

		private:

			IHost*									m_host;
			std::unique_ptr<std::thread>			m_thread;
			std::vector<_T*>						m_objects;
			std::atomic_bool						m_stopEvent;

			Stats									m_writeStats;
			Stats									m_readStats;
			std::mutex								m_readStatsLock;
			std::vector<uint32_t>					m_stateCounters;
			Timer									m_updateTimer;

			static void
			_ThreadMain(
				Thread*		aThread)
			{
				while(!aThread->m_stopEvent)
				{
					for(_T* t : aThread->m_objects)
						t->Update(aThread->m_host, aThread->m_writeStats);

					if(aThread->m_updateTimer.HasExpired())
					{
						for(uint32_t& stateCounter : aThread->m_stateCounters)
							stateCounter = 0;

						{
							std::lock_guard lock(aThread->m_readStatsLock);
							
							aThread->m_readStats.Add(aThread->m_writeStats);

							for (_T* t : aThread->m_objects)
								t->UpdateStateCounters(aThread->m_stateCounters);
						}

						aThread->m_writeStats.Reset();
					}

					std::this_thread::yield();
				}
			}
		};

		template <typename _T>
		class ThreadCollection
		{
		public:
			ThreadCollection(
				IHost*				aHost,
				uint32_t			aNumThreads,
				std::vector<_T*>&	aObjects)
			{
				for(uint32_t i = 0; i < aNumThreads; i++)
					m_threads.push_back(new Thread<_T>(aHost));

				size_t threadIndex = 0;

				for(_T* t : aObjects)
				{
					m_threads[threadIndex]->Add(t);

					threadIndex = (threadIndex + 1) % m_threads.size();
				}

				for(Thread<_T>* t : m_threads)
					t->Start();
			}

			~ThreadCollection()
			{
				for(Thread<_T>* t : m_threads)
					delete t;
			}

			void
			PrintStats(
				const char*	aHeader)
			{
				printf("\n--- %s ---\n", aHeader);

				Stats combinedStats;
				_T::InitStats(combinedStats);

				std::vector<uint32_t> stateCounters;
				stateCounters.resize((size_t)_T::GetNumStates());

				for (Thread<_T>* t : m_threads)
				{
					Stats threadStats;
					_T::InitStats(threadStats);

					t->GetStats(threadStats, stateCounters);

					combinedStats.Add(threadStats);
				}

				_T::PrintStats(combinedStats, stateCounters);
			}

		private:

			std::vector<Thread<_T>*>	m_threads;
		};

	}

	void		
	SimTest(
		const char*			aWorkingDirectory,
		const Config*		aConfig)
	{
		Network network(aWorkingDirectory, aConfig);

		network.m_host.DeleteAllFiles(UINT32_MAX);

		{
			ThreadCollection<Client> clients(&network.m_host, aConfig->m_simNumClientThreads, network.m_clients);
			ThreadCollection<GameServer> gameServers(&network.m_host, aConfig->m_simNumGameServerThreads, network.m_gameServers);
			ThreadCollection<BlobServer::BlobServerType> blobServers(&network.m_host, aConfig->m_simNumBlobServerThreads, network.m_blobServers);
			ThreadCollection<LockServer::LockServerType> lockServers(&network.m_host, aConfig->m_simNumLockServerThreads, network.m_lockServers);

			for(;;)
			{
				system("clear");

				clients.PrintStats("CLIENTS");
				gameServers.PrintStats("GAME_SERVERS");
				blobServers.PrintStats("BLOB_SERVERS");
				lockServers.PrintStats("LOCK_SERVERS");

				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	}

}