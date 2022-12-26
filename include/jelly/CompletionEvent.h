#pragma once

#include <atomic>
#include <functional>

namespace jelly
{

	struct CompletionEvent
	{
		CompletionEvent()
			: m_signal(false)
		{

		}
		
		void
		Signal()
		{
			m_signal = true;
			if(m_callback)
				m_callback();
		}

		bool
		Poll()
		{
			return m_signal;
		}

		// Public data
		std::atomic_bool		m_signal;
		std::function<void()>	m_callback;
	};

}