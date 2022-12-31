#pragma once

namespace jelly
{

	// Generic intrusive double-linked list type
	template <typename _Type>
	class List
	{
	public:
		List()
			: m_head(NULL)
			, m_tail(NULL)
		{

		}

		void
		Add(
			_Type*	aItem)
		{
			JELLY_ASSERT(aItem->m_next == NULL);
			JELLY_ASSERT(aItem->m_prev == NULL);

			aItem->m_prev = m_tail;

			if (m_tail != NULL)
				m_tail->m_next = aItem;
			else
				m_head = aItem;

			m_tail = aItem;
		}

		void
		Remove(
			_Type*	aItem)
		{
			if (aItem->m_next != NULL)
				aItem->m_next->m_prev = aItem->m_prev;
			else
				m_tail = aItem->m_prev;

			if (aItem->m_prev != NULL)
				aItem->m_prev->m_next = aItem->m_next;
			else
				m_head = aItem->m_next;

			aItem->m_next = NULL;
			aItem->m_prev = NULL;
		}

		void
		MoveToTail(
			_Type*	aItem)
		{
			Remove(aItem);
			Add(aItem);
		}

		bool
		IsEmpty() const
		{
			return m_head == NULL && m_tail == NULL;
		}

		// Public data
		_Type*		m_head;
		_Type*		m_tail;
	};

}