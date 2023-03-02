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
		Signal() noexcept
		{
			m_signal = true;
		}

		//! Poll if the completion event has been signaled
		bool
		Poll() const noexcept
		{
			return m_signal;
		}

		//! Reset the completion event after it has been signaled
		void
		Reset() noexcept
		{
			m_signal = false;
		}

		//-----------------------------------------------------------------------------
		// Public data

		std::atomic_bool		m_signal;
	};

}