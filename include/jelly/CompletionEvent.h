#pragma once

namespace jelly
{

	/**
	 * \brief Event object that can be signaled when something has completed.
	 */
	struct CompletionEvent
	{
		CompletionEvent()
			: m_signal(false)
		{

		}
		
		//! Signal the completion event
		void
		Signal()
		{
			m_signal = true;
			if(m_callback)
				m_callback();
		}

		//! Poll if the completion event has been signaled
		bool
		Poll() const
		{
			return m_signal;
		}

		//! Reset the completion event after it has been signaled
		void
		Reset()
		{
			m_signal = false;
		}

		//-----------------------------------------------------------------------------
		// Public data

		std::atomic_bool		m_signal;
		std::function<void()>	m_callback; //!< Optional callback to be called when signaled. This is done on the signaling thread.
	};

}