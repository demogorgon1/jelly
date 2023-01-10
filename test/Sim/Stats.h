#pragma once

#include <jelly/ErrorUtils.h>

namespace jelly::Test::Sim
{

	struct Stats
	{
		enum Type
		{
			TYPE_COUNTER,
			TYPE_SAMPLE
		};

		static void
		PrintStateCounter(
			const char*						aName,
			uint32_t						aState,
			const std::vector<uint32_t>&	aStateCounters)
		{
			JELLY_ASSERT((size_t)aState < aStateCounters.size());
			if(aStateCounters[aState] > 0)
				printf("[STATE]%s : %u\n", aName, aStateCounters[aState]);
		}

		struct Entry
		{
			Entry()
				: m_count(0)
				, m_sum(0)
				, m_min(0)
				, m_max(0)
			{

			}

			void
			Sample(
				uint32_t		aValue)
			{
				JELLY_ASSERT(m_count < UINT32_MAX);
				JELLY_ASSERT(UINT32_MAX - m_sum >= aValue);
				m_count++;
				m_sum += aValue;
				m_min = std::min(m_min, aValue);
				m_max = std::max(m_max, aValue);
			}

			void
			Reset()
			{
				m_count = 0;
				m_sum = 0;
				m_min = 0;
				m_max = 0;
			}

			void
			Add(
				const Entry&	aOther)
			{
				if(aOther.m_count > 0)
				{
					JELLY_ASSERT(m_count + aOther.m_count < UINT32_MAX);
					JELLY_ASSERT(UINT32_MAX - m_sum >= aOther.m_sum);
					m_count += aOther.m_count;
					m_sum += aOther.m_sum;
					m_min = std::min(m_min, aOther.m_min);
					m_max = std::max(m_max, aOther.m_max);
				}
			}

			// Public data
			uint32_t		m_count;
			uint32_t		m_sum;
			uint32_t		m_min;
			uint32_t		m_max;
		};

		Stats()
		{

		}

		~Stats()
		{

		}

		void
		Init(
			uint32_t		aNumEntries)
		{
			m_entries.resize((size_t)aNumEntries);
		}

		void
		Sample(
			uint32_t		aIndex,
			uint32_t		aValue = 0)
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			m_entries[aIndex].Sample(aValue);
		}

		void
		Reset()
		{
			for(Entry& t : m_entries)
				t.Reset();
		}

		void
		Add(
			const Stats&	aOther)
		{
			JELLY_ASSERT(m_entries.size() == aOther.m_entries.size());
			for(size_t i = 0; i < m_entries.size(); i++)
				m_entries[i].Add(aOther.m_entries[i]);
		}

		void
		Print(
			Type			aType,
			uint32_t		aIndex,
			const char*		aName) const
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			const Entry& t = m_entries[aIndex];

			if(aType == TYPE_COUNTER)
			{
				printf("%-32s : %u\n", aName, t.m_count);
			}
			else
			{
				if (t.m_count == 0)
					printf("%-32s : N/A\n", aName);
				else
					printf("%-32s : [avg:%8u][min:%8u][max:%8u]\n", aName, t.m_sum / t.m_count, t.m_min, t.m_max);
			}
		}

		// Public data
		std::vector<Entry>		m_entries;
	};

}