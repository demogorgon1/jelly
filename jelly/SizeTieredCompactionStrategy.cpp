#include <jelly/Base.h>

#include <jelly/ConfigProxy.h>
#include <jelly/ErrorUtils.h>

#include "SizeTieredCompactionStrategy.h"

namespace jelly
{

	namespace
	{

		struct Store
		{
			Store(
				const IHost::StoreInfo&						aStoreInfo) noexcept
				: m_storeId(aStoreInfo.m_id)
				, m_size(aStoreInfo.m_size)
			{

			}

			uint32_t						m_storeId;
			size_t							m_size;
		};

		struct Bucket
		{
			Bucket() noexcept
				: m_size(0)
			{

			}

			std::vector<Store>				m_stores;
			size_t							m_size;
		};

		struct BucketList
		{
			BucketList() noexcept
			{

			}

			~BucketList()
			{
				for(Bucket* t : m_buckets)
					delete t;
			}

			Bucket*
			FindSuitableBucketBySize(
				size_t										aSize,
				size_t										aAvailableDiskSpace) noexcept
			{
				for(Bucket* t : m_buckets)
				{
					JELLY_ASSERT(t->m_stores.size() > 0);

					size_t bucketAverageSize = t->m_size / t->m_stores.size();
					size_t bucketRangeMin = bucketAverageSize / 2;
					size_t bucketRangeMax = (bucketAverageSize * 3) / 2;
					
					if(t->m_size + aSize < aAvailableDiskSpace && aSize > bucketRangeMin && aSize < bucketRangeMax)
						return t;
				}

				return NULL;
			}

			void
			AddStoreToSuitableBucket(
				const Store&								aStore,
				size_t										aAvailableDiskSpace) noexcept
			{
				if(aStore.m_size > aAvailableDiskSpace)
					return;

				Bucket* bucket = FindSuitableBucketBySize(aStore.m_size, aAvailableDiskSpace);
				if(bucket == NULL)
					m_buckets.push_back(bucket = new Bucket());


				bucket->m_stores.push_back(aStore);
				bucket->m_size += aStore.m_size;
			}

			// Public data
			std::vector<Bucket*>			m_buckets;
		};

	}

	SizeTieredCompactionStrategy::SizeTieredCompactionStrategy(
		ConfigProxy*										aConfig) noexcept
		: m_config(aConfig)
	{

	}
	
	SizeTieredCompactionStrategy::~SizeTieredCompactionStrategy()
	{

	}

	//------------------------------------------------------------------------------------

	void	
	SizeTieredCompactionStrategy::Update(
		const std::vector<IHost::StoreInfo>&				aStoreInfo,
		const TotalStoreSize&								/*aTotalStoreSize*/,
		size_t												aAvailableDiskSpace,
		SuggestionCallback									aSuggestionCallback)
	{
		JELLY_ASSERT(aStoreInfo.size() > 0);

		size_t minBucketSizeToCompact = m_config->GetSize(Config::ID_STCS_MIN_BUCKET_SIZE);

		// Divide stores into buckets of roughly equal sizes
		BucketList bucketList;
		for(const IHost::StoreInfo& storeInfo : aStoreInfo)
			bucketList.AddStoreToSuitableBucket(storeInfo, aAvailableDiskSpace);

		// Find buckets ripe for compaction
		for(const Bucket* bucket : bucketList.m_buckets)
		{
			if(bucket->m_stores.size() >= (size_t)minBucketSizeToCompact)
			{
				CompactionJob compactionJob;
				compactionJob.m_oldestStoreId = aStoreInfo[0].m_id;

				for(const Store& store : bucket->m_stores)
					compactionJob.m_storeIds.push_back(store.m_storeId);
				
				aSuggestionCallback(compactionJob);
			}
		}
	}

}