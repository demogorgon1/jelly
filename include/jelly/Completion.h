#pragma once

#include "CompletionEvent.h"
#include "ErrorUtils.h"
#include "Exception.h"
#include "RequestResult.h"

namespace jelly
{

	struct Completion
	{
		typedef std::function<void()> Callback;

		Completion() noexcept
			: m_result(REQUEST_RESULT_NONE)
			, m_exception(0)
		{

		}

		void
		Signal() noexcept
		{
			JELLY_ASSERT(m_result != REQUEST_RESULT_NONE);
			JELLY_ASSERT(!m_completed.Poll());

			if(m_callback)
				m_callback();

			m_completed.Signal();
		}

		void
		OnException(
			Exception::Code		aException) noexcept
		{
			JELLY_ASSERT(!m_completed.Poll());

			m_result = REQUEST_RESULT_EXCEPTION;
			m_exception = aException;
			
			Signal();
		}

		void
		OnCancel() noexcept
		{
			JELLY_ASSERT(!m_completed.Poll());

			m_result = REQUEST_RESULT_CANCELED;
			
			Signal();
		}

		// Public data
		RequestResult					m_result;		
		Exception::Code					m_exception;
		CompletionEvent					m_completed;		
		Callback						m_callback;
	};

}