#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>

#include "SizeTieredCompactionStrategy.h"

namespace jelly
{

	namespace
	{

		struct Store
		{
			Store(
				const IHost::StoreInfo&						aStoreInfo)
				: m_storeId(aStoreInfo.m_id)
				, m_size(aStoreInfo.m_size)
			{

			}

			uint32_t						m_storeId;
			size_t							m_size;
		};

		struct Bucket
		{
			Bucket()
				: m_size(0)
			{

			}

			std::vector<Store>				m_stores;
			size_t							m_size;
		};

		struct BucketList
		{
			BucketList()
			{

			}

			~BucketList()
			{
				for(Bucket* t : m_buckets)
					delete t;
			}

			Bucket*
			FindSuitableBucketBySize(
				size_t										aSize)
			{
				for(Bucket* t : m_buckets)
				{
					JELLY_ASSERT(t->m_stores.size() > 0);
					size_t bucketAverageSize = t->m_size / t->m_stores.size();
					size_t bucketRangeMin = bucketAverageSize / 2;
					size_t bucketRangeMax = (bucketAverageSize * 3) / 2;
					
					if(aSize > bucketRangeMin && aSize < bucketRangeMax)
						return t;
				}

				return NULL;
			}

			void
			AddStoreToSuitableBucket(
				const Store&								aStore)
			{
				Bucket* bucket = FindSuitableBucketBySize(aStore.m_size);
				if(bucket == NULL)
					m_buckets.push_back(bucket = new Bucket());

				bucket->m_stores.push_back(aStore);
				bucket->m_size += aStore.m_size;
			}

			// Public data
			std::vector<Bucket*>			m_buckets;
		};

	}

	SizeTieredCompactionStrategy::SizeTieredCompactionStrategy()
	{

	}
	
	SizeTieredCompactionStrategy::~SizeTieredCompactionStrategy()
	{

	}

	//------------------------------------------------------------------------------------

	void	
	SizeTieredCompactionStrategy::Update(
		const std::vector<IHost::StoreInfo>&	aStoreInfo,
		const TotalStoreSize&					/*aTotalStoreSize*/,
		SuggestionCallback						aSuggestionCallback)
	{
		JELLY_ASSERT(aStoreInfo.size() > 0);

		// Divide stores into buckets of roughly equal sizes
		BucketList bucketList;
		for(const IHost::StoreInfo& storeInfo : aStoreInfo)
			bucketList.AddStoreToSuitableBucket(storeInfo);

		// Find buckets ripe for compaction
		std::vector<const Bucket*> candidateBuckets;

		for(const Bucket* bucket : bucketList.m_buckets)
		{
			if(bucket->m_stores.size() >= 2)
				candidateBuckets.push_back(bucket);
		}

		if(candidateBuckets.size() > 0)
		{
			// Pick a random candidate bucket and compact two stores from it
			// FIXME: do something more advanced

			std::uniform_int_distribution<size_t> uniformDistribution(0, candidateBuckets.size() - 1);
			const Bucket* bucket = candidateBuckets[uniformDistribution(m_random)];

			const Store& store1 = bucket->m_stores[0];
			const Store& store2 = bucket->m_stores[1];

			uint32_t oldestStoreId = aStoreInfo[0].m_id;

			aSuggestionCallback(CompactionJob(oldestStoreId, store1.m_storeId, store2.m_storeId));
		}
	}

}