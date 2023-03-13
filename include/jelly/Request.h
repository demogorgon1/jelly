#pragma once

#include "Completion.h"

namespace jelly
{

	/**
	 * \brief Base class for BlobNode and LockNode requests
	 */
	template <typename _RequestType>
	class Request
	{
	public:
		Request() noexcept
			: m_next(NULL)
			, m_timeStamp(0)
			, m_hasPendingWrite(false)
			, m_lowPrio(false)
		{

		}

		void
		SetResult(
			RequestResult			aResult) noexcept
		{
			m_completion.m_result = aResult;
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
		SetLowPrio(
			bool					aLowPrio) noexcept
		{
			m_lowPrio = aLowPrio;
		}

		void
		Execute() noexcept
		{
			JELLY_ASSERT(m_executionCallback);
			JELLY_ASSERT(m_completion.m_result == REQUEST_RESULT_NONE);
			JELLY_ASSERT(!IsCompleted());

			try
			{
				m_executionCallback();
			}
			catch(Exception::Code e)
			{
				m_completion.m_exception = e;

				m_completion.m_result = REQUEST_RESULT_EXCEPTION;
			}				
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
			if(m_lowPrio)
			{
				// Signal as completed immediately after being queued up in the WAL
				aNode->WriteToWAL(aItem, true, NULL);
			}
			else
			{
				// Not going to be signaled as completed before WAL has been flushed
				m_hasPendingWrite = true;

				aNode->WriteToWAL(aItem, false, &m_completion);
			}
		}

		/**
		 * \brief Sets a callback to be called on completion. Don't make any assumptions about which thread
		 * this is called on. The callback must not throw any exceptions.
		 */
		void
		SetCompletionCallback(
			std::function<void()>	aCallback) noexcept
		{
			m_completion.m_callback = aCallback;
		}

		//---------------------------------------------------------------------------------------------------
		// Data access

		bool			IsCompleted() const noexcept { return m_completion.m_completed.Poll(); }	//!< Poll request for completion.
		RequestResult	GetResult() const noexcept { return m_completion.m_result; }				//!< Returns result after completion.
		Exception::Code	GetException() const noexcept { return m_completion.m_exception; }			//!< Returns exception if GetResult() is RESULT_EXCEPTION.
		uint64_t		GetTimeStamp() const noexcept { return m_timeStamp; }						//!< Returns time stamp after completion.
		bool			IsLowPrio() const noexcept { return m_lowPrio; }							//!< Returns low-priority request flag.

		_RequestType*	GetNext() noexcept { return m_next; }
		bool			HasPendingWrite() const noexcept { return m_hasPendingWrite; }
		Completion*		GetCompletion() noexcept { return &m_completion; }

	protected:
		
		uint64_t						m_timeStamp;
		Completion						m_completion;
		bool							m_hasPendingWrite;
		bool							m_lowPrio;
		_RequestType*					m_next;
		std::function<void()>			m_executionCallback;
	};

}