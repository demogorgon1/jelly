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
			m_stats->Emit(m_id, m_perfTimer.GetElapsedMicroseconds(), Stat::TYPE_SAMPLER);
		}

	private:
	
		PerfTimer	m_perfTimer;
		IStats*		m_stats;
		uint32_t	m_id;
	};
	
}