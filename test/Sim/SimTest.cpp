#include <memory>
#include <thread>

#include "../Config.h"

#include "Client.h"
#include "BlobServer.h"
#include "GameServer.h"
#include "LockServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{
	
	namespace
	{

		template <typename _T>
		class Thread
		{
		public:
			Thread()
			{
				
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

			std::unique_ptr<std::thread>			m_thread;
			std::vector<_T*>						m_objects;
			std::atomic_bool						m_stopEvent;

			static void
			_ThreadMain(
				Thread*		aThread)
			{
				while(!aThread->m_stopEvent)
				{
					for(_T* t : aThread->m_objects)
						t->Update();

					std::this_thread::yield();
				}
			}
		};

		template <typename _T>
		class ThreadCollection
		{
		public:
			ThreadCollection(
				uint32_t			aNumThreads,
				std::vector<_T*>&	aObjects)
			{
				for(uint32_t i = 0; i < aNumThreads; i++)
					m_threads.push_back(new Thread<_T>());

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
			ThreadCollection<Client> clients(aConfig->m_simNumClientThreads, network.m_clients);
			ThreadCollection<GameServer> gameServers(aConfig->m_simNumGameServerThreads, network.m_gameServers);
			ThreadCollection<BlobServer> blobServers(aConfig->m_simNumBlobServerThreads, network.m_blobServers);
			ThreadCollection<LockServer> lockServers(aConfig->m_simNumLockServerThreads, network.m_lockServers);

			for(;;)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

}