#pragma once

#include "CompletionEvent.h"
#include "Result.h"

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
			: m_result(RESULT_NONE)
			, m_next(NULL)
			, m_timeStamp(0)
			, m_hasPendingWrite(false)
		{

		}

		void
		SetResult(
			Result					aResult)
		{
			m_result = aResult;
		}

		void
		SetTimeStamp(
			uint64_t				aTimeStamp)
		{
			m_timeStamp = aTimeStamp;
		}

		void
		SetExecutionCallback(
			std::function<void()>	aExecutionCallback)
		{
			m_executionCallback = aExecutionCallback;
		}

		void
		Execute()
		{
			JELLY_ASSERT(m_executionCallback);
			JELLY_ASSERT(m_result == RESULT_NONE);
			JELLY_ASSERT(!IsCompleted());

			m_executionCallback();
		}

		void
		SignalCompletion()
		{
			JELLY_ASSERT(m_result != RESULT_NONE);
			JELLY_ASSERT(!IsCompleted());

			m_completed.Signal();
		}

		void
		SetNext(
			_RequestType*			aNext)
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

		bool			IsCompleted() const { return m_completed.Poll(); }		//!< Poll request for completion.
		Result			GetResult() const { return m_result; }					//!< Returns result after completion.
		uint64_t		GetTimeStamp() const { return m_timeStamp; }			//!< Returns time stamp after completion.

		_RequestType*	GetNext() { return m_next; }
		bool			HasPendingWrite() const { return m_hasPendingWrite; }

	protected:
		
		uint64_t						m_timeStamp;
		Result							m_result;			
		CompletionEvent					m_completed;		
		bool							m_hasPendingWrite;
		_RequestType*					m_next;
		std::function<void()>			m_executionCallback;
	};

}