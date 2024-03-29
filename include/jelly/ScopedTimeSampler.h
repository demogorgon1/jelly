#pragma once

#include "IStats.h"
#include "PerfTimer.h"

namespace jelly
{

	// Starts a high-resolution timer and emits it as a statistic sampler when it runs out of scope.
	class ScopedTimeSampler
	{
	public:
		ScopedTimeSampler(
			IStats*		aStats,
			uint32_t	aId) noexcept
			: m_stats(aStats)
			, m_id(aId)
		{

		}

		~ScopedTimeSampler()
		{
			if(m_id != UINT32_MAX)
				m_stats->Emit(m_id, m_perfTimer.GetElapsedMicroseconds(), Stat::TYPE_SAMPLER);
		}

	private:
	
		PerfTimer	m_perfTimer;
		IStats*		m_stats;
		uint32_t	m_id;
	};
	
}