#pragma once

#include "ItemBase.h"
#include "IWriter.h"
#include "LockMetaData.h"

namespace jelly
{

	class WAL;

	// Item stored by LockNode
	template <typename _KeyType, typename _LockType, typename _LockMetaType>
	class LockNodeItem
		: public ItemBase
	{		
	public:
		struct RuntimeState
		{
			RuntimeState()
				: m_pendingWAL(NULL)
				, m_walInstanceCount(0)
			{

			}

			WAL*								m_pendingWAL;
			uint32_t							m_walInstanceCount;
		};

		LockNodeItem(
			const _KeyType&									aKey = _KeyType(),
			const _LockType&								aLock = _LockType())
			: m_key(aKey)
			, m_lock(aLock)
		{

		}

		LockNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			const _LockType&								aLock,			
			const _LockMetaType&							aMeta,
			uint32_t										aTombstoneStoreId = UINT32_MAX)
			: m_key(aKey)
			, m_lock(aLock)
			, m_meta(aMeta)
		{
			SetSeq(aSeq);
			SetTombstoneStoreId(aTombstoneStoreId);
		}

		void
		Reset()
		{
			JELLY_ASSERT(m_runtimeState.m_pendingWAL == NULL);
			JELLY_ASSERT(m_runtimeState.m_walInstanceCount == 0);

			m_key = _KeyType();
			m_lock = _LockType();
			m_meta = _LockMetaType();

			ResetBase();
		}

		void
		MoveFrom(
			LockNodeItem*									aOther)
		{
			m_key = aOther->m_key;
			m_lock = aOther->m_lock;
			m_meta = aOther->m_meta;

			CopyBase(*aOther);
		}

		bool
		Compare(
			const LockNodeItem*								aOther) const
		{
			return m_key == aOther->m_key 
				&& m_lock == aOther->m_lock 
				&& m_meta == aOther->m_meta
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

		void
		CompactionUpdate(
			uint32_t										/*aNewStoreId*/,
			size_t											/*aNewStoreOffset*/)
		{
		}

		void
		SetKey(
			const _KeyType&									aKey)
		{
			m_key = aKey;
		}

		void
		SetLock(
			const _LockType&								aLock)
		{
			m_lock = aLock;
		}

		void
		SetMeta(
			const _LockMetaType&							aMeta)
		{
			m_meta = aMeta;
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

		// Data access
		const _KeyType&			GetKey() const { return m_key; }
		const _LockType&		GetLock() const { return m_lock; }
		_LockType&				GetLock() { return m_lock; }
		const _LockMetaType&	GetMeta() const { return m_meta; }
		const RuntimeState&		GetRuntimeState() const { return m_runtimeState; }
		RuntimeState&			GetRuntimeState() { return m_runtimeState; }

	private:

		_KeyType							m_key;
		_LockType							m_lock;
		_LockMetaType						m_meta;
		RuntimeState						m_runtimeState;
	};

}