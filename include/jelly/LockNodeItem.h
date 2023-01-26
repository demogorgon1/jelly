#pragma once

#include "ItemBase.h"
#include "IWriter.h"
#include "MetaData.h"

namespace jelly
{

	class WAL;

	// Item stored by LockNode
	template <typename _KeyType, typename _LockType>
	struct LockNodeItem
		: public ItemBase
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
			SetSeq(aSeq);
			SetTombstoneStoreId(aTombstoneStoreId);

			m_meta.m_blobSeq = aBlobSeq;
			m_meta.m_blobNodeIds = aBlobNodeIds;
		}

		void
		Reset()
		{
			JELLY_ASSERT(m_pendingWAL == NULL);
			JELLY_ASSERT(m_walInstanceCount == 0);

			m_key = _KeyType();
			m_lock = _LockType();

			ResetBase();
		}

		void
		MoveFrom(
			LockNodeItem*									aOther)
		{
			m_key = aOther->m_key;
			m_lock = aOther->m_lock;

			m_meta.m_blobNodeIds = aOther->m_meta.m_blobNodeIds;
			m_meta.m_blobSeq = aOther->m_meta.m_blobSeq;

			CopyBase(*aOther);
		}

		bool
		Compare(
			const LockNodeItem*								aOther) const
		{
			return m_key == aOther->m_key 
				&& m_lock == aOther->m_lock 
				&& m_meta.m_blobNodeIds == aOther->m_meta.m_blobNodeIds
				&& m_meta.m_blobSeq == aOther->m_meta.m_blobSeq 
				&& CompareBase(*aOther);
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
			if (!ShouldBePruned(aOldestStoreId))
				aStoreWriter->WriteItem(this);

			return UINT64_MAX;
		}

		// IItem implementation
		size_t
		Write(
			IWriter*										aWriter) const override
		{
			WriteBase(aWriter);
			m_key.Write(aWriter);			
			m_lock.Write(aWriter);
			m_meta.Write(aWriter);
		
			return 0; // Only used for blobs
		}

		bool
		Read(
			IReader*										aReader,
			size_t*											/*aOutBlobOffset*/) override
		{
			if (!ReadBase(aReader))
				return false;
			if (!m_key.Read(aReader))
				return false;
			if (!m_lock.Read(aReader))
				return false;
			if(!m_meta.Read(aReader))
				return false;

			return true;
		}

		// Public data
		_KeyType							m_key;
		_LockType							m_lock;	
		MetaData::Lock						m_meta;

		// Runtime state, not serialized
		WAL*								m_pendingWAL;
		uint32_t							m_walInstanceCount;
	};

}