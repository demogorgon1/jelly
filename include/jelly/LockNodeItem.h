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
			, m_walInstanceCount(0)
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
			, m_walInstanceCount(0)
		{
			m_meta.m_seq = aSeq;
			m_meta.m_blobSeq = aBlobSeq;
			m_meta.m_blobNodeIds = aBlobNodeIds;
			m_tombstone.m_storeId = aTombstoneStoreId;
		}

		void
		MoveFrom(
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

		bool
		CompactionRead(
			IFileStreamReader*								aStoreReader)
		{
			return Read(aStoreReader, NULL);
		}

		uint64_t
		CompactionWrite(
			uint32_t										aOldestStoreId,
			IStoreWriter*									aStoreWriter)
		{
			if (!m_tombstone.ShouldPrune(aOldestStoreId))
				aStoreWriter->WriteItem(this);

			return UINT64_MAX;
		}
		// IItem implementation
		size_t
		Write(
			IWriter*										aWriter) const override
		{
			m_key.Write(aWriter);			
			m_lock.Write(aWriter);
			m_meta.Write(aWriter);
			m_tombstone.Write(aWriter);

			return 0; // Only used for blobs
		}

		bool
		Read(
			IReader*										aReader,
			size_t*											/*aOutBlobOffset*/) override
		{
			if (!m_key.Read(aReader))
				return false;
			if (!m_lock.Read(aReader))
				return false;
			if(!m_meta.Read(aReader))
				return false;
			if(!m_tombstone.Read(aReader))
				return false;
			return true;
		}

		// Public data
		_KeyType							m_key;
		_LockType							m_lock;	
		MetaData::Lock						m_meta;
		MetaData::Tombstone					m_tombstone;

		// Runtime state, not serialized
		WAL*								m_pendingWAL;
		uint32_t							m_walInstanceCount;
	};

}