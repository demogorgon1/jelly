#pragma once

#include <thread>

namespace jelly::Test::Sim
{

	class SharedWorkQueue;

	class SharedWorkerThread
	{
	public:
						SharedWorkerThread(
							SharedWorkQueue*	aQueue);
						~SharedWorkerThread();

		void			BlockingStop();

	private:

		SharedWorkQueue*				m_queue;
		std::unique_ptr<std::thread>	m_thread;
		std::atomic_bool				m_stopEvent;

		static void		_ThreadMain(
							SharedWorkerThread* aThread);

		void			_Main();
	};

}