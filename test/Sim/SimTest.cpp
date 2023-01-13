#include <thread>

#include <jelly/API.h>

#include "../Config.h"

#include "BlobServer.h"
#include "Client.h"
#include "CSVOutput.h"
#include "GameServer.h"
#include "LockServer.h"
#include "Network.h"
#include "Stats.h"

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

				m_stateInfo.resize(_T::GetNumStates());

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
				Stats&						aOutStats,
				std::vector<Stats::Entry>&	aOutStateInfo)
			{
				std::lock_guard lock(m_readStatsLock);

				aOutStats = m_readStats;
				m_readStats.Reset();

				JELLY_ASSERT(m_stateInfo.size() == aOutStateInfo.size());
				for(size_t i = 0; i < m_stateInfo.size(); i++)
					aOutStateInfo[i].Add(m_stateInfo[i]);
			}

		private:

			IHost*									m_host;
			std::unique_ptr<std::thread>			m_thread;
			std::vector<_T*>						m_objects;
			std::atomic_bool						m_stopEvent;

			Stats									m_writeStats;
			Stats									m_readStats;
			std::mutex								m_readStatsLock;
			std::vector<Stats::Entry>				m_stateInfo;
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
						for(Stats::Entry& stateCounter : aThread->m_stateInfo)
							stateCounter.Reset();

						{
							std::lock_guard lock(aThread->m_readStatsLock);
							
							for (_T* t : aThread->m_objects)
								t->UpdateStateInfo(aThread->m_writeStats, aThread->m_stateInfo);

							aThread->m_readStats.Add(aThread->m_writeStats);
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
			InitCSV(
				const char*	aColumnPrefix,
				CSVOutput*	aCSV)
			{
				JELLY_ASSERT(m_csvColumnPrefix.empty());
				m_csvColumnPrefix = aColumnPrefix;

				_T::InitCSV(aColumnPrefix, aCSV);
			}

			void
			PrintStats(
				const char*		aHeader,
				CSVOutput*		aCSV,
				const Config*	aConfig)
			{
				if(aConfig->m_simTestStdOut)
					printf("\n--- %s ---\n", aHeader);

				Stats combinedStats;
				_T::InitStats(combinedStats);

				std::vector<Stats::Entry> stateCounters;
				stateCounters.resize((size_t)_T::GetNumStates());

				for (Thread<_T>* t : m_threads)
				{
					Stats threadStats;
					_T::InitStats(threadStats);

					t->GetStats(threadStats, stateCounters);

					combinedStats.Add(threadStats);
				}

				_T::PrintStats(combinedStats, stateCounters, aCSV, m_csvColumnPrefix.c_str(), aConfig);
			}

		private:

			std::vector<Thread<_T>*>	m_threads;
			std::string					m_csvColumnPrefix;
		};

	}

	void		
	SimTest(
		const char*			aWorkingDirectory,
		const Config*		aConfig)
	{
		Network network(aWorkingDirectory, aConfig);

		network.m_host.DeleteAllFiles(UINT32_MAX);

		std::unique_ptr<CSVOutput> csv;

		if(!aConfig->m_simCSVOutput.empty() && aConfig->m_simCSVOutputColumns.size() > 0)
		{
			csv = std::make_unique<CSVOutput>(aConfig->m_simCSVOutput.c_str());

			for(const std::string& column : aConfig->m_simCSVOutputColumns)
				csv->ShowColumn(column.c_str());
		}

		{
			ThreadCollection<Client> clients(&network.m_host, aConfig->m_simNumClientThreads, network.m_clients);
			ThreadCollection<GameServer> gameServers(&network.m_host, aConfig->m_simNumGameServerThreads, network.m_gameServers);
			ThreadCollection<BlobServer::BlobServerType> blobServers(&network.m_host, aConfig->m_simNumBlobServerThreads, network.m_blobServers);
			ThreadCollection<LockServer::LockServerType> lockServers(&network.m_host, aConfig->m_simNumLockServerThreads, network.m_lockServers);

			if(csv)
			{
				clients.InitCSV("C", csv.get());
				gameServers.InitCSV("G", csv.get());
				blobServers.InitCSV("B", csv.get());
				lockServers.InitCSV("L", csv.get());
			}			

			Timer infoTimer(1000);

			for(;;)
			{
				if(infoTimer.HasExpired())
				{
					if(csv)
						csv->StartNewRow();
			
					if(aConfig->m_simTestStdOut)
						system("clear");

					clients.PrintStats("CLIENTS", csv.get(), aConfig);
					gameServers.PrintStats("GAME_SERVERS", csv.get(), aConfig);
					blobServers.PrintStats("BLOB_SERVERS", csv.get(), aConfig);
					lockServers.PrintStats("LOCK_SERVERS", csv.get(), aConfig);

					if(csv)
						csv->Flush();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50)); // It's not going to be exactly every 1 second, but close enough (tm)
			}
		}
	}

}