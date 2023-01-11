#pragma once

#include "CompletionEvent.h"
#include "Result.h"

namespace jelly
{

	// Base class for BlobNode and LockNode requests
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

		// These are all internals and shouldn't be set by the application
		uint64_t						m_timeStamp;
		Result							m_result;
		bool							m_hasPendingWrite;
		CompletionEvent					m_completed;
		_RequestType*					m_next;
		std::function<void()>			m_callback;
	};

}