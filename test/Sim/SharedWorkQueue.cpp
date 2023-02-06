#include <jelly/API.h>

#include "SharedWorkQueue.h"

namespace jelly::Test::Sim
{

	SharedWorkQueue::SharedWorkQueue()
	{

	}

	SharedWorkQueue::~SharedWorkQueue()
	{

	}

	void	
	SharedWorkQueue::PostWork(
		Work	aWork)
	{
		std::lock_guard lock(m_lock);
		m_queue.push_back(aWork);
	}

	bool	
	SharedWorkQueue::GetWork(
		Work&	aOutWork)
	{
		bool hasWork = false;

		{
			std::lock_guard lock(m_lock);

			if (m_queue.size() > 0)
			{
				aOutWork = m_queue.front();
				m_queue.pop_front();
				hasWork = true;
			}
		}

		if(!hasWork)
		{
			// FIXME: should really have some kinda of semaphore or something instead, but doesn't really matter too much
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		return hasWork;
	}

}