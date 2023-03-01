#pragma once

#include "CompletionEvent.h"
#include "RequestResult.h"

namespace jelly
{

	/**
	 * \brief Base class for BlobNode and LockNode requests
	 */
	template <typename _RequestType>
	class Request
	{
	public:
		Request()
			: m_result(REQUEST_RESULT_NONE)
			, m_next(NULL)
			, m_timeStamp(0)
			, m_hasPendingWrite(false)
			, m_exception(0)
		{

		}

		void
		SetResult(
			RequestResult			aResult) noexcept
		{
			m_result = aResult;
		}

		void
		SetTimeStamp(
			uint64_t				aTimeStamp) noexcept
		{
			m_timeStamp = aTimeStamp;
		}

		void
		SetExecutionCallback(
			std::function<void()>	aExecutionCallback) noexcept
		{
			m_executionCallback = aExecutionCallback;
		}

		void
		Execute()
		{
			JELLY_ASSERT(m_executionCallback);
			JELLY_ASSERT(m_result == REQUEST_RESULT_NONE);
			JELLY_ASSERT(!IsCompleted());

			try
			{
				m_executionCallback();
			}
			catch(Exception::Code e)
			{
				m_exception = e;

				m_result = REQUEST_RESULT_EXCEPTION;
				
				SignalCompletion();
			}				
		}

		void
		SignalCompletion() noexcept
		{
			JELLY_ASSERT(m_result != REQUEST_RESULT_NONE);
			JELLY_ASSERT(!IsCompleted());

			m_completed.Signal();
		}

		void
		SetNext(
			_RequestType*			aNext) noexcept
		{
			JELLY_ASSERT(m_next == NULL);

			m_next = aNext;
		}

		template <typename _NodeType, typename _ItemType>
		void
		WriteToWAL(
			_NodeType*				aNode,
			_ItemType*				aItem)
		{
			m_hasPendingWrite = true;

			aNode->WriteToWAL(aItem, &m_completed, &m_result);
		}

		//---------------------------------------------------------------------------------------------------
		// Data access

		bool			IsCompleted() const noexcept { return m_completed.Poll(); }		//!< Poll request for completion.
		RequestResult	GetResult() const noexcept { return m_result; }					//!< Returns result after completion.
		uint64_t		GetTimeStamp() const noexcept { return m_timeStamp; }			//!< Returns time stamp after completion.
		Exception::Code	GetException() const noexcept { return m_exception; }			//!< Returns exception if GetResult() is RESULT_EXCEPTION.

		_RequestType*	GetNext() noexcept { return m_next; }
		bool			HasPendingWrite() const noexcept { return m_hasPendingWrite; }

	protected:
		
		uint64_t						m_timeStamp;
		RequestResult					m_result;		
		Exception::Code					m_exception;
		CompletionEvent					m_completed;		
		bool							m_hasPendingWrite;
		_RequestType*					m_next;
		std::function<void()>			m_executionCallback;
	};

}