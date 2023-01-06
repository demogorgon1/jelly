#pragma once

#include "IItem.h"
#include "IWriter.h"
#include "MetaData.h"

namespace jelly
{

	class WAL;

	// Item stored by LockNode
	template <typename _KeyType, typename _LockType>
	struct LockNodeItem
		: public IItem
	{		
		LockNodeItem(
			const _KeyType&									aKey = _KeyType(),
			const _LockType&								aLock = _LockType())
			: m_key(aKey)
			, m_lock(aLock)
			, m_pendingWAL(NULL)
		{

		}

		LockNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			const _LockType&								aLock,			
			uint32_t										aBlobSeq,
			uint32_t										aBlobNodeIds,
			uint32_t										aTombstoneStoreId = UINT32_MAX)
			: m_key(aKey)
			, m_lock(aLock)
			, m_pendingWAL(NULL)
		{
			m_meta.m_seq = aSeq;
			m_meta.m_blobSeq = aBlobSeq;
			m_meta.m_blobNodeIds = aBlobNodeIds;
			m_tombstone.m_storeId = aTombstoneStoreId;
		}

		void
		CopyFrom(
			LockNodeItem*									aOther)
		{
			m_key = aOther->m_key;
			m_lock = aOther->m_lock;
			m_tombstone = aOther->m_tombstone;
			m_meta.m_seq = aOther->m_meta.m_seq;
			m_meta.m_blobNodeIds = aOther->m_meta.m_blobNodeIds;
			m_meta.m_blobSeq = aOther->m_meta.m_blobSeq;
			m_meta.m_timeStamp = aOther->m_meta.m_timeStamp;
		}

		bool
		Compare(
			const LockNodeItem*								aOther) const
		{
			// Note: not comparing timestamps
			return m_key == aOther->m_key 
				&& m_lock == aOther->m_lock 
				&& m_meta.m_seq == aOther->m_meta.m_seq
				&& m_meta.m_blobNodeIds == aOther->m_meta.m_blobNodeIds
				&& m_meta.m_blobSeq == aOther->m_meta.m_blobSeq 
				&& m_tombstone == aOther->m_tombstone;
		}

		// IItem implementation
		void
		Write(
			IWriter*										aWriter,
			const Compression::IProvider*					/*aItemCompression*/) const override
		{
			JELLY_CHECK(m_key.Write(aWriter), "Failed to write lock item key.");
			JELLY_CHECK(m_lock.Write(aWriter), "Failed to write lock item lock.");
			JELLY_CHECK(aWriter->Write(&m_meta, sizeof(m_meta)) == sizeof(m_meta), "Failed to write lock item meta data.");
			JELLY_CHECK(aWriter->Write(&m_tombstone, sizeof(m_tombstone)) == sizeof(m_tombstone), "Failed to write lock item tombstone data.");
		}

		bool
		Read(
			IReader*										aReader,
			const Compression::IProvider*					/*aItemCompression*/,
			ReadType										aReadType = READ_TYPE_ALL) override
		{
			JELLY_ASSERT(aReadType == READ_TYPE_ALL);

			if (!m_key.Read(aReader))
				return false;
			if (!m_lock.Read(aReader))
				return false;
			if (aReader->Read(&m_meta, sizeof(m_meta)) != sizeof(m_meta))
				return false;
			if (aReader->Read(&m_tombstone, sizeof(m_tombstone)) != sizeof(m_tombstone))
				return false;
			return true;
		}

		size_t
		GetStoredBlobSize() const
		{
			JELLY_ASSERT(false);
			return 0;
		}

		// Public data
		_KeyType							m_key;
		_LockType							m_lock;	
		MetaData::Lock						m_meta;
		MetaData::Tombstone					m_tombstone;

		// Runtime state, not serialized
		WAL*								m_pendingWAL;
	};

}