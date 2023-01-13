#pragma once

#include "CompactionStrategy.h"
#include "IHost.h"

namespace jelly
{

	class IHost;

	class CompactionAdvisor
	{
	public:
							CompactionAdvisor(
								uint32_t								aNodeId,
								IHost*									aHost,
								uint32_t								aTotalSizeMemory,
								uint32_t								aTotalSizeTrendMemory);
							~CompactionAdvisor();

		void				Update();
		CompactionStrategy	GetNextSuggestion();

	private:

		uint32_t											m_nodeId;
		IHost*												m_host;

		std::chrono::time_point<std::chrono::steady_clock>	m_lastCompactionTimeStamps[NUM_COMPACTION_STRATEGIES];

		struct Internal;
		std::unique_ptr<Internal>							m_internal;

		// Ring-buffer for holding sugestions
		static const size_t MAX_SUGGESTIONS = 16;
		CompactionStrategy									m_suggestionBuffer[MAX_SUGGESTIONS];
		size_t												m_suggestionBufferReadOffset;
		size_t												m_suggestionBufferWriteOffset;
		size_t												m_suggestionCount;

		struct TotalStoreSize
		{
			TotalStoreSize()
				: m_c(0)
				, m_dc(0)
				, m_ddc(0)
			{

			}

			size_t											m_c;	// Current
			int32_t											m_dc;	// Trend of m_c
			int32_t											m_ddc;	// Trend of m_dc;
		};

		void				_AddSuggestion(
								CompactionStrategy						aSuggestion);
		void				_UpdateCompactionStrategy(
								CompactionStrategy						aCompactionStrategy,
								const std::vector<IHost::StoreInfo>&	aStoreInfo,
								const TotalStoreSize&					aTotalStoreSize);
	};

}