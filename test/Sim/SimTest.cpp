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

		private:

			IHost*									m_host;
			std::unique_ptr<std::thread>			m_thread;
			std::vector<_T*>						m_objects;
			std::atomic_bool						m_stopEvent;

			Timer									m_updateTimer;
			std::vector<uint32_t>					m_stateCounters;

			static void
			_ThreadMain(
				Thread*		aThread)
			{
				while(!aThread->m_stopEvent)
				{
					for(_T* t : aThread->m_objects)
						t->Update();

					if(aThread->m_updateTimer.HasExpired())
					{
						std::fill(aThread->m_stateCounters.begin(), aThread->m_stateCounters.end(), 0);

						for (_T* t : aThread->m_objects)
							t->UpdateStateStatistics(aThread->m_stateCounters);

						for(size_t i = 0; i < aThread->m_stateCounters.size(); i++)
							aThread->m_host->GetStats()->Emit(_T::GetStateNumStatsId((uint32_t)i), aThread->m_stateCounters[i], Stat::TYPE_GAUGE);
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
			csv = std::make_unique<CSVOutput>(aConfig->m_simCSVOutput.c_str(), network.m_host.GetStats());

			for(const std::string& column : aConfig->m_simCSVOutputColumns)
				csv->AddColumn(column.c_str());
		}

		{
			ThreadCollection<Client> clients(&network.m_host, aConfig->m_simNumClientThreads, network.m_clients);
			ThreadCollection<GameServer> gameServers(&network.m_host, aConfig->m_simNumGameServerThreads, network.m_gameServers);
			ThreadCollection<BlobServer::BlobServerType> blobServers(&network.m_host, aConfig->m_simNumBlobServerThreads, network.m_blobServers);
			ThreadCollection<LockServer::LockServerType> lockServers(&network.m_host, aConfig->m_simNumLockServerThreads, network.m_lockServers);

			Timer statsTimer(1000);

			for(;;)
			{
				if(statsTimer.HasExpired())
				{
					network.m_host.PollSystemStats();

					IStats* stats = network.m_host.GetStats();
					stats->Update();

					if(csv)
						csv->WriteRow();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(50)); // It's not going to be exactly every 1 second, but close enough (tm)
			}
		}
	}

}