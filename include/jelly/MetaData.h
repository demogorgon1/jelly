#pragma once

namespace jelly
{

	// Item meta data as stored on disk (both stores and WALs)

	namespace MetaData
	{

		struct Lock
		{
			Lock()
				: m_seq(0)
				, m_timeStamp(0)
				, m_blobSeq(UINT32_MAX)
				, m_blobNodeIds(UINT32_MAX)
			{
			}

			// Public data
			uint64_t							m_timeStamp;
			uint32_t							m_seq;
			uint32_t							m_blobSeq; // UINT32_MAX means not set
			uint32_t							m_blobNodeIds; // 4 uint8_t packed into 1 uint32_t (UINT8_MAX means not set)
		};

		struct Blob
		{
			Blob()
				: m_seq(0)
				, m_timeStamp(0)
			{
			}

			// Public data
			uint64_t							m_timeStamp;
			uint32_t							m_seq;
		};

		struct Tombstone
		{
			Tombstone()
				: m_storeId(UINT32_MAX)
			{

			}

			void
			Clear()
			{
				m_storeId = UINT32_MAX;
			}

			void
			Set(
				uint32_t			aStoreId)
			{
				JELLY_ASSERT(aStoreId != UINT32_MAX);
				m_storeId = aStoreId;
			}

			bool
			ShouldPrune(
				uint32_t			aOldestStoreId) const
			{
				return m_storeId != UINT32_MAX && aOldestStoreId > m_storeId;
			}

			bool
			IsSet() const
			{
				return m_storeId != UINT32_MAX;
			}

			bool
			operator ==(
				const Tombstone&	aOther) const
			{
				return m_storeId == aOther.m_storeId;
			}

			// Public data
			uint32_t							m_storeId;
		};

	}

}