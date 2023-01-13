#pragma once

namespace jelly
{

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

		bool
		IsSet() const
		{
			return m_oldestStoreId != UINT32_MAX && m_storeId1 != UINT32_MAX && m_storeId2 != UINT32_MAX;
		}

		// Public data
		uint32_t		m_oldestStoreId;
		uint32_t		m_storeId1;
		uint32_t		m_storeId2;
	};

}