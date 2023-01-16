#pragma once

#include "ErrorUtils.h"

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
			, m_count(0)
		{

		}

		void
		Add(
			_Type*	aItem)
		{
			#if defined(JELLY_EXTRA_CONSISTENCY_CHECKS)
				JELLY_ASSERT(!Has(aItem));
			#endif
			
			JELLY_ASSERT(aItem->m_next == NULL);
			JELLY_ASSERT(aItem->m_prev == NULL);

			aItem->m_prev = m_tail;

			if (m_tail != NULL)
				m_tail->m_next = aItem;
			else
				m_head = aItem;

			m_tail = aItem;

			m_count++;
		}

		void
		Remove(
			_Type*	aItem)
		{
			#if defined(JELLY_EXTRA_CONSISTENCY_CHECKS)
				JELLY_ASSERT(Has(aItem));
			#endif

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

			JELLY_ASSERT(m_count > 0);
			m_count--;
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
			return m_count == 0;
		}

		bool
		Has(
			const _Type*	aItem) const
		{
			for(_Type* i = m_head; i != NULL; i = i->m_next)
			{
				if(i == aItem)
					return true;
			}
			return false;
		}

		// Public data
		_Type*		m_head;
		_Type*		m_tail;
		size_t		m_count;
	};

}