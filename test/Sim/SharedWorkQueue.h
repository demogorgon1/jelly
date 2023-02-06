#pragma once

#include <list>

namespace jelly::Test::Sim
{

	class SharedWorkQueue
	{
	public:
		typedef std::function<void()> Work;

				SharedWorkQueue();
				~SharedWorkQueue();

		void	PostWork(
					Work	aWork);
		bool	GetWork(
					Work&	aOutWork);
	
	private:

		std::mutex						m_lock;
		std::list<Work>					m_queue;
	};

}