#include <jelly/API.h>

#include "SharedWorkerThread.h"
#include "SharedWorkQueue.h"

namespace jelly::Test::Sim
{

	SharedWorkerThread::SharedWorkerThread(
		SharedWorkQueue*		aQueue) 
		: m_queue(aQueue)
		, m_stopEvent(false)
	{
		m_thread = std::make_unique<std::thread>(_ThreadMain, this);
	}
	
	SharedWorkerThread::~SharedWorkerThread()
	{
		BlockingStop();
	}

	void			
	SharedWorkerThread::BlockingStop()
	{
		if(m_thread)
		{
			m_stopEvent = true;
			m_thread->join();
			m_thread.reset();
		}
	}

	//-----------------------------------------------------------------------------------

	void		
	SharedWorkerThread::_ThreadMain(
		SharedWorkerThread*		aThread)
	{
		aThread->_Main();
	}

	//-----------------------------------------------------------------------------------

	void			
	SharedWorkerThread::_Main()
	{
		while(!m_stopEvent)
		{
			SharedWorkQueue::Work work;
			if(m_queue->GetWork(work))
				work();
		}
	}

}