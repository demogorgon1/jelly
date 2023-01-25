#pragma once

#include "CompletionEvent.h"
#include "Result.h"

namespace jelly
{

	/**
	 * Base class for BlobNode and LockNode requests
	 */
	template <typename _RequestType>
	struct Request
	{
		Request()
			: m_result(RESULT_NONE)
			, m_next(NULL)
			, m_timeStamp(0)
			, m_hasPendingWrite(false)
		{

		}

		//--------------------------------------------------------------------------------
		// Public data

		// These are all internals and shouldn't be set by the application
		uint64_t						m_timeStamp;
		Result							m_result;			//!< When completed this holds the result of the request.
		bool							m_hasPendingWrite;
		CompletionEvent					m_completed;		//!< This will be signaled when the request has completed.
		_RequestType*					m_next;
		std::function<void()>			m_callback;
	};

}