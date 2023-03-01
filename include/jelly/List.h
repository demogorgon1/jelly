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
			_Type*	aItem) noexcept
		{
			#if defined(JELLY_EXTRA_CONSISTENCY_CHECKS)
				JELLY_ASSERT(!Has(aItem));
			#endif
			
			JELLY_ASSERT(aItem->GetNext() == NULL);
			JELLY_ASSERT(aItem->GetPrev() == NULL);

			aItem->SetPrev(m_tail);

			if (m_tail != NULL)
				m_tail->SetNext(aItem);
			else
				m_head = aItem;

			m_tail = aItem;

			m_count++;
		}

		void
		Remove(
			_Type*	aItem) noexcept
		{
			#if defined(JELLY_EXTRA_CONSISTENCY_CHECKS)
				JELLY_ASSERT(Has(aItem));
			#endif

			if (aItem->GetNext() != NULL)
				aItem->GetNext()->SetPrev(aItem->GetPrev());
			else
				m_tail = aItem->GetPrev();

			if (aItem->GetPrev() != NULL)
				aItem->GetPrev()->SetNext(aItem->GetNext());
			else
				m_head = aItem->GetNext();

			aItem->SetNext(NULL);
			aItem->SetPrev(NULL);

			JELLY_ASSERT(m_count > 0);
			m_count--;
		}

		void
		MoveToTail(
			_Type*	aItem) noexcept
		{
			Remove(aItem);
			Add(aItem);
		}

		bool
		IsEmpty() const noexcept
		{
			return m_count == 0;
		}

		bool
		Has(
			const _Type*	aItem) const noexcept
		{
			for(_Type* i = m_head; i != NULL; i = i->GetNext())
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