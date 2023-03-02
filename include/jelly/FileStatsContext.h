#pragma once

namespace jelly
{

	class IStats;

	// Tells a File object how I/O statistics should be reported for this specific file.
	struct FileStatsContext
	{
		FileStatsContext(
			IStats* aStats) noexcept
			: m_stats(aStats)
			, m_idRead(UINT32_MAX)
			, m_idWrite(UINT32_MAX) 
		{

		}

		IStats*		m_stats;
		uint32_t	m_idWrite;
		uint32_t	m_idRead;
	};


}