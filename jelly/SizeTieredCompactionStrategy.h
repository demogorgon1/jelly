#pragma once

#include "ICompactionStrategy.h"

namespace jelly
{

	// Size-tiered compaction strategy (STCS) for use with the compaction advisor	
	class SizeTieredCompactionStrategy
		: public ICompactionStrategy
	{
	public:
				SizeTieredCompactionStrategy(
					uint32_t								aMinBucketSizeToCompact);
		virtual	~SizeTieredCompactionStrategy();

		// ICompactionStrategy implementation
		void	Update(
					const std::vector<IHost::StoreInfo>&	aStoreInfo,
					const TotalStoreSize&					aTotalStoreSize,
					size_t									aAvailableDiskSpace,
					SuggestionCallback						aSuggestionCallback) override;

	private:

		uint32_t		m_minBucketSizeToCompact;
	};
	
}