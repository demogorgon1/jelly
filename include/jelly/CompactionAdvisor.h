#pragma once

#include "CompactionJob.h"
#include "IHost.h"
#include "Timer.h"

namespace jelly
{

	class IHost;

	// Helper for advising about compactions. This is used internally by the HousekeepingAdvisor.
	class CompactionAdvisor
	{
	public:
		enum Strategy
		{
			STRATEGY_SIZE_TIERED
		};

							CompactionAdvisor(
								uint32_t								aNodeId,
								IHost*									aHost,
								ConfigProxy*							aConfig,
								Strategy								aStrategy);
							~CompactionAdvisor();

		void				Update();
		CompactionJob		GetNextSuggestion() noexcept;

	private:

		uint32_t											m_nodeId;
		IHost*												m_host;

		Timer												m_compactionStrategyUpdateCooldown;

		struct Internal;
		std::unique_ptr<Internal>							m_internal;

		// Ring-buffer for holding sugestions
		static const size_t MAX_SUGGESTIONS = 16;
		CompactionJob										m_suggestionBuffer[MAX_SUGGESTIONS];
		size_t												m_suggestionBufferReadOffset;
		size_t												m_suggestionBufferWriteOffset;
		size_t												m_suggestionCount;

		void				_AddSuggestion(
								const CompactionJob&					aSuggestion) noexcept;
	};

}