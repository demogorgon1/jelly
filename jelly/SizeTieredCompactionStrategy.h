#pragma once

#include "ICompactionStrategy.h"

namespace jelly
{

	// Size-tiered compaction strategy (STCS) for use with the compaction advisor	
	class SizeTieredCompactionStrategy
		: public ICompactionStrategy
	{
	public:
				SizeTieredCompactionStrategy();
		virtual	~SizeTieredCompactionStrategy();

		// ICompactionStrategy implementation
		void	Update(
					const std::vector<IHost::StoreInfo>&	aStoreInfo,
					const TotalStoreSize&					aTotalStoreSize,
					SuggestionCallback						aSuggestionCallback) override;

	private:

		std::mt19937	m_random;
	};
	
}