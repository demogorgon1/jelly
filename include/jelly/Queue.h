#pragma once

namespace jelly
{

	// Node request queue
	template <typename _RequestType>
	struct Queue
	{
		Queue()
			: m_first(NULL)
			, m_last(NULL)
			, m_guard(false)
			, m_count(0)
		{

		}

		~Queue()
		{		
			// Can't delete the queue if it has pending requests
			JELLY_ASSERT(m_first == NULL);
			JELLY_ASSERT(m_last == NULL);
			JELLY_ASSERT(m_count == 0);
		}

		void
		Add(
			_RequestType*	aRequest)
		{
			JELLY_ASSERT(!m_guard);

			if (m_last != NULL)
				m_last->m_next = aRequest;
			else
				m_first = aRequest;

			m_last = aRequest;

			m_count++;
		}

		void
		SetGuard()
		{
			JELLY_ASSERT(!m_guard);
			m_guard = true;
		}

		void
		Reset()
		{
			m_guard = false;
			m_first = NULL;
			m_last = NULL;
			m_count = 0;
		}

		// Public data
		bool				m_guard;
		_RequestType*		m_first;
		_RequestType*		m_last;
		uint32_t			m_count;
	};

}