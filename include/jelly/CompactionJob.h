#pragma once

namespace jelly
{

	/** 
	* \brief Description of a minor compaction job (some stores turned into one). 
	* 
	* The "oldest store id" is required for pruning tombstones: every tombstone has an 
	* inscription mentioning the latest store id at time of deletion. When a tombstone
	* is encountered during the compaction process, the inscriped store id is compared 
	* with the current oldest one. If the oldest one is newer it means that no present 
	* store can possilby have references to this item - so it can safely be deleted.
	*/
	struct CompactionJob
	{
		CompactionJob(
			uint32_t	aOldestStoreId = UINT32_MAX,
			uint32_t	aStoreId1 = UINT32_MAX,
			uint32_t	aStoreId2 = UINT32_MAX)
			: m_oldestStoreId(aOldestStoreId)
		{
			if(aStoreId1 != UINT32_MAX)
				m_storeIds.push_back(aStoreId1);

			if (aStoreId2 != UINT32_MAX)
				m_storeIds.push_back(aStoreId2);
		}

		//! Validates if job is defined correctly
		bool
		Validate() const
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
		IsSet() const
		{
			return m_oldestStoreId != UINT32_MAX && m_storeIds.size() > 0;
		}

		//-----------------------------------------------------------------------
		// Public data
		uint32_t				m_oldestStoreId; //!< Oldest store id currently present on disk
		std::vector<uint32_t>	m_storeIds;		 //!< Store ids to perform compaction on
	};

}