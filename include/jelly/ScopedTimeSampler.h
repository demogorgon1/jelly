#pragma once

#include "IStats.h"
#include "PerfTimer.h"

namespace jelly
{

	class ScopedTimeSampler
	{
	public:
		ScopedTimeSampler(
			IStats*		aStats,
			uint32_t	aId)
			: m_stats(aStats)
			, m_id(aId)
		{

		}

		~ScopedTimeSampler()
		{
			m_stats->SampleTime(m_id, m_perfTimer.GetElapsedMicroseconds());
		}

	private:
	
		PerfTimer	m_perfTimer;
		IStats*		m_stats;
		uint32_t	m_id;
	};
	
}