#pragma once

namespace jelly
{

	/** 
	* \brief Description of a minor compaction job (some stores turned into one). 
	* 
	* Tombstones are only pruned if the job includes all stores, as stores made by compactions
	* get new ids: a store that isn't included could have an older version of a deleted item,
	* even if its id is newer than the tombstone's. The "oldest store id" isn't used for this.
	*/
	struct CompactionJob
	{
		CompactionJob(
			uint32_t	aOldestStoreId = UINT32_MAX,
			uint32_t	aStoreId1 = UINT32_MAX,
			uint32_t	aStoreId2 = UINT32_MAX) noexcept
			: m_oldestStoreId(aOldestStoreId) 
		{
			if(aStoreId1 != UINT32_MAX)
				m_storeIds.push_back(aStoreId1);

			if (aStoreId2 != UINT32_MAX)
				m_storeIds.push_back(aStoreId2);
		}

		//! Validates if job is defined correctly
		bool
		Validate() const noexcept
		{
			if(m_oldestStoreId == UINT32_MAX)
				return false;

			if(m_storeIds.size() < 2)
				return false;

			std::unordered_set<uint32_t> storeIdSet;
			for(uint32_t storeId : m_storeIds)
			{
				if(storeIdSet.find(storeId) != storeIdSet.end())
					return false;
			}
			return true;
		}

		//! Indicates if this is a valid compaction job
		bool
		IsSet() const noexcept
		{
			return m_oldestStoreId != UINT32_MAX && m_storeIds.size() > 0;
		}

		//-----------------------------------------------------------------------
		// Public data
		uint32_t				m_oldestStoreId; //!< Oldest store id currently present on disk
		std::vector<uint32_t>	m_storeIds;		 //!< Store ids to perform compaction on
	};

}