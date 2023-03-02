#pragma once

#include "ICompactionStrategy.h"

namespace jelly
{

	class ConfigProxy;

	// Size-tiered compaction strategy (STCS) for use with the compaction advisor	
	class SizeTieredCompactionStrategy
		: public ICompactionStrategy
	{
	public:
				SizeTieredCompactionStrategy(
					ConfigProxy*							aConfig) noexcept;
		virtual	~SizeTieredCompactionStrategy();

		// ICompactionStrategy implementation
		void	Update(
					const std::vector<IHost::StoreInfo>&	aStoreInfo,
					const TotalStoreSize&					aTotalStoreSize,
					size_t									aAvailableDiskSpace,
					SuggestionCallback						aSuggestionCallback) override;

	private:

		ConfigProxy*	m_config;
	};
	
}