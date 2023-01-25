#pragma once

namespace jelly
{

	/** 
	* Description of a minor compaction job (two stores turned into one). 
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
			, m_storeId1(aStoreId1)
			, m_storeId2(aStoreId2)
		{

		}

		//! Indicates if this is a valid compaction job
		bool
		IsSet() const
		{
			return m_oldestStoreId != UINT32_MAX && m_storeId1 != UINT32_MAX && m_storeId2 != UINT32_MAX;
		}

		//-----------------------------------------------------------------------
		// Public data
		uint32_t		m_oldestStoreId; //!< Oldest store id currently present on disk
		uint32_t		m_storeId1;		 //!< First store id
		uint32_t		m_storeId2;		 //!< Second store id
	};

}