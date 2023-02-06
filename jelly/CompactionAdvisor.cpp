#include <jelly/Base.h>

#include <jelly/CompactionAdvisor.h>
#include <jelly/ErrorUtils.h>

#include "RingBuffer.h"
#include "SizeTieredCompactionStrategy.h"

namespace jelly
{

	namespace
	{

		constexpr int32_t
		_GetSizeDifference(
			size_t								aSize1,
			size_t								aSize2)
		{
			if(aSize1 < aSize2)
			{
				size_t d = aSize2 - aSize1;
				if(d <= INT32_MAX)
					return (int32_t)d;
				else
					return INT32_MAX;
			}
			else
			{
				size_t d = aSize1 - aSize2;
				if(d < (size_t)(-(int64_t)INT32_MIN))
					return (int32_t)-((int64_t)d);
				else
					return INT32_MIN;
			}
		}

		// Make sure the logic of _GetSizeDifference() is sound
		static_assert(_GetSizeDifference(10, 20) == 10);
		static_assert(_GetSizeDifference(20, 10) == -10);
		static_assert(_GetSizeDifference(10, ((size_t)UINT32_MAX) * 2) == INT32_MAX);
		static_assert(_GetSizeDifference(((size_t)UINT32_MAX) * 2, 10) == INT32_MIN);
	}

	//------------------------------------------------------------------------

	struct CompactionAdvisor::Internal
	{
		Internal(
			uint32_t							aTotalSizeMemory,
			uint32_t							aTotalSizeTrendMemory,
			uint32_t							aSTCSMinBucketSize,
			Strategy							aStrategy)
			: m_totalSizeBuffer((size_t)aTotalSizeMemory)
			, m_totalSizeTrendBuffer((size_t)aTotalSizeTrendMemory)
		{
			switch(aStrategy)
			{
			case STRATEGY_SIZE_TIERED:			m_compactionStrategy = std::make_unique<SizeTieredCompactionStrategy>(aSTCSMinBucketSize); break;
			
			default:				
				JELLY_FATAL_ERROR("Invalid compaction strategy.");
			}
		}

		RingBuffer<size_t>						m_totalSizeBuffer;
		RingBuffer<int32_t>						m_totalSizeTrendBuffer;
		std::unique_ptr<ICompactionStrategy>	m_compactionStrategy;
	};

	//------------------------------------------------------------------------

	CompactionAdvisor::CompactionAdvisor(
		uint32_t								aNodeId,
		IHost*									aHost,
		uint32_t								aTotalSizeMemory,
		uint32_t								aTotalSizeTrendMemory,
		uint32_t								aMinCompactionStrategyUpdateIntervalMS,
		uint32_t								aSTCSMinBucketSize,
		Strategy								aStrategy)
		: m_host(aHost)
		, m_nodeId(aNodeId)
		, m_suggestionBufferReadOffset(0)
		, m_suggestionBufferWriteOffset(0)
		, m_suggestionCount(0)
	{
		memset(m_suggestionBuffer, 0, sizeof(m_suggestionBuffer));

		m_internal = std::make_unique<Internal>(aTotalSizeMemory, aTotalSizeTrendMemory, aSTCSMinBucketSize, aStrategy);

		m_compactionStrategyUpdateCooldown.SetTimeout(aMinCompactionStrategyUpdateIntervalMS);
	}
	
	CompactionAdvisor::~CompactionAdvisor()
	{

	}

	void				
	CompactionAdvisor::Update()
	{
		// Query information about stores from host
		std::vector<IHost::StoreInfo> storeInfo;
		m_host->GetStoreInfo(m_nodeId, storeInfo);

		if (storeInfo.size() > 0)
		{
			// Remove latest store because it might be in the process of being created
			storeInfo.resize(storeInfo.size() - 1);
		}

		// Update store size buffers and calculate trend and derivative
		ICompactionStrategy::TotalStoreSize totalStoreSize;

		{
			for (const IHost::StoreInfo& t : storeInfo)
				totalStoreSize.m_c += t.m_size;

			m_internal->m_totalSizeBuffer.Add(totalStoreSize.m_c);

			totalStoreSize.m_dc = _GetSizeDifference(m_internal->m_totalSizeBuffer.GetOldestValue(), totalStoreSize.m_c);

			m_internal->m_totalSizeTrendBuffer.Add(totalStoreSize.m_dc);

			totalStoreSize.m_ddc = m_internal->m_totalSizeTrendBuffer.GetOldestValue() - totalStoreSize.m_dc;
		}

		// Generate suggestions (if we have room for more and have enough stores)
		if(m_suggestionCount < MAX_SUGGESTIONS && storeInfo.size() >= 2 && m_compactionStrategyUpdateCooldown.HasExpired())
		{
			m_internal->m_compactionStrategy->Update(storeInfo, totalStoreSize, m_host->GetAvailableDiskSpace(), [&](
				const CompactionJob& aSuggestion)
			{
				_AddSuggestion(aSuggestion);
			});
		}
	}
	
	CompactionJob
	CompactionAdvisor::GetNextSuggestion()
	{
		if(m_suggestionCount == 0)
			return CompactionJob();

		CompactionJob suggestion = m_suggestionBuffer[m_suggestionBufferReadOffset];
		m_suggestionBufferReadOffset = (m_suggestionBufferReadOffset + 1) % MAX_SUGGESTIONS;
		m_suggestionCount--;

		return suggestion;
	}

	//------------------------------------------------------------------------

	void				
	CompactionAdvisor::_AddSuggestion(
		const CompactionJob&					aSuggestion)
	{
		if(m_suggestionCount < MAX_SUGGESTIONS)
		{
			m_suggestionBuffer[m_suggestionBufferWriteOffset] = aSuggestion;
			m_suggestionBufferWriteOffset = (m_suggestionBufferWriteOffset + 1) % MAX_SUGGESTIONS;
			m_suggestionCount++;
		}
	}

}