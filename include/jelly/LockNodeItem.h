#pragma once

#include <assert.h>

#include "IItem.h"
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
			uint32_t										aBlobNodeIds)
			: m_key(aKey)
			, m_lock(aLock)
			, m_pendingWAL(NULL)
		{
			m_meta.m_seq = aSeq;
			m_meta.m_blobSeq = aBlobSeq;
			m_meta.m_blobNodeIds = aBlobNodeIds;
		}

		void
		CopyFrom(
			LockNodeItem*									aOther)
		{
			m_key = aOther->m_key;
			m_lock = aOther->m_lock;
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
				&& m_meta.m_blobSeq == aOther->m_meta.m_blobSeq;
		}

		// IItem implementation
		bool
		Write(
			IWriter*										aWriter,
			const Compression::IProvider*					/*aItemCompression*/) const override
		{
			if (!m_key.Write(aWriter))
				return false;
			if (!m_lock.Write(aWriter))
				return false;
			if (aWriter->Write(&m_meta, sizeof(m_meta)) != sizeof(m_meta))
				return false;
			return true;
		}

		bool
		Read(
			IReader*										aReader,
			const Compression::IProvider*					/*aItemCompression*/,
			ReadType										aReadType = READ_TYPE_ALL) override
		{
			assert(aReadType == READ_TYPE_ALL);

			if (!m_key.Read(aReader))
				return false;
			if (!m_lock.Read(aReader))
				return false;
			if (aReader->Read(&m_meta, sizeof(m_meta)) != sizeof(m_meta))
				return false;
			return true;
		}

		size_t
		GetStoredBlobSize() const
		{
			assert(false);
			return 0;
		}

		// Public data
		_KeyType							m_key;
		_LockType							m_lock;	
		MetaData::Lock						m_meta;

		// Runtime state, not serialized
		WAL*								m_pendingWAL;
	};

}