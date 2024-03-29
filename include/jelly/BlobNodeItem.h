#pragma once

#include "Compression.h"
#include "ErrorUtils.h"
#include "IBuffer.h"
#include "IFileStreamReader.h"
#include "IReader.h"
#include "ItemBase.h"
#include "IStoreWriter.h"
#include "IWriter.h"
#include "MetaData.h"

namespace jelly
{

	class WAL;

	// Blob node item
	template <typename _KeyType, typename _MetaType>
	class BlobNodeItem
		: public ItemBase
	{
	public:
		struct RuntimeState
		{
			RuntimeState() noexcept
				: m_pendingWAL(NULL)
				, m_next(NULL)
				, m_prev(NULL)
				, m_storeId(0)
				, m_storeOffset(0)
				, m_storeSize(0)
				, m_walInstanceCount(0)
				, m_isResident(false) 
			{
			
			}

			WAL*								m_pendingWAL;
			uint32_t							m_walInstanceCount;
			uint32_t							m_storeId;
			size_t								m_storeOffset;
			size_t								m_storeSize;
			bool								m_isResident;

			// Placement in timestamp-sorted linked list used to expell oldest items when memory limit is reached
			BlobNodeItem<_KeyType, _MetaType>*	m_next;
			BlobNodeItem<_KeyType, _MetaType>*	m_prev;
		};

		BlobNodeItem(
			const _KeyType&									aKey = _KeyType(),
			uint32_t										aSeq = 0) noexcept
			: m_key(aKey)
			, m_lockSeq(0)
		{
			SetSeq(aSeq);
		}

		BlobNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			IBuffer*										aBlob) noexcept
			: m_key(aKey)
			, m_lockSeq(0)
		{
			SetSeq(aSeq);

			m_blob.reset(aBlob);
		}

		void
		SetKey(
			const _KeyType&									aKey) noexcept
		{
			m_key = aKey;
		}

		void
		SetBlob(
			IBuffer*										aBlob) noexcept
		{
			m_blob.reset(aBlob);
		}

		void
		SetMeta(
			const _MetaType&								aMeta) noexcept
		{
			m_meta = aMeta;
		}

		void
		SetLockSeq(
			uint32_t										aLockSeq) noexcept
		{
			m_lockSeq = aLockSeq;
		}

		void
		Reset() noexcept
		{
			JELLY_ASSERT(m_runtimeState.m_pendingWAL == NULL);
			JELLY_ASSERT(m_runtimeState.m_next == NULL);
			JELLY_ASSERT(m_runtimeState.m_prev == NULL);
			JELLY_ASSERT(m_runtimeState.m_walInstanceCount == 0);
			JELLY_ASSERT(!m_runtimeState.m_isResident);

			m_key = _KeyType();
			m_meta = _MetaType();
			m_lockSeq = 0;

			m_runtimeState.m_storeId = 0;
			m_runtimeState.m_storeOffset = 0;
			m_runtimeState.m_storeSize = 0;

			ResetBase();
		}

		void
		MoveFrom(
			BlobNodeItem*									aOther) noexcept
		{
			m_blob = std::move(aOther->m_blob);

			m_key = aOther->m_key;
			m_meta = aOther->m_meta;
			m_lockSeq = aOther->m_lockSeq;

			m_runtimeState.m_storeId = aOther->m_runtimeState.m_storeId;
			m_runtimeState.m_storeOffset = aOther->m_runtimeState.m_storeOffset;
			m_runtimeState.m_storeSize = aOther->m_runtimeState.m_storeSize;
			m_runtimeState.m_isResident = aOther->m_runtimeState.m_isResident;

			CopyBase(*aOther);
		}

		bool
		CompactionRead(
			IFileStreamReader*								aStoreReader) 
		{
			m_runtimeState.m_storeOffset = aStoreReader->GetTotalBytesRead();

			return Read(aStoreReader, &m_runtimeState.m_storeOffset);
		}

		uint64_t
		CompactionWrite(
			uint32_t										aOldestStoreId,
			IStoreWriter*									aStoreWriter)
		{
			m_runtimeState.m_storeOffset = UINT64_MAX;

			if (!ShouldBePruned(aOldestStoreId))
				m_runtimeState.m_storeOffset = aStoreWriter->WriteItem(this);

			return m_runtimeState.m_storeOffset;
		}

		void
		CompactionUpdate(
			uint32_t										aNewStoreId,
			size_t											aNewStoreOffset) noexcept
		{
			m_runtimeState.m_storeId = aNewStoreId;
			m_runtimeState.m_storeOffset = aNewStoreOffset;
		}

		BlobNodeItem<_KeyType, _MetaType>*
		GetNext() noexcept
		{
			return m_runtimeState.m_next;
		}

		BlobNodeItem<_KeyType, _MetaType>*
		GetPrev() noexcept
		{
			return m_runtimeState.m_prev;
		}

		void
		SetNext(
			BlobNodeItem<_KeyType, _MetaType>*				aNext) noexcept
		{
			m_runtimeState.m_next = aNext;
		}

		void
		SetPrev(
			BlobNodeItem<_KeyType, _MetaType>*				aPrev) noexcept
		{
			m_runtimeState.m_prev = aPrev;
		}

		// IItem implementation
		size_t
		Write(
			IWriter*										aWriter) const override
		{
			size_t startOffset = aWriter->GetTotalBytesWritten();

			WriteBase(aWriter);
			m_key.Write(aWriter);
			m_meta.Write(aWriter);
			aWriter->WriteUInt(m_lockSeq);

			size_t blobOffset = 0; 

			if(!HasTombstone())
			{
				JELLY_ASSERT(m_blob);

				aWriter->WriteUInt<size_t>(m_blob->GetSize());

				blobOffset = aWriter->GetTotalBytesWritten() - startOffset;

				aWriter->Write(m_blob->GetPointer(), m_blob->GetSize());
			}
			else
			{
				JELLY_ASSERT(!m_blob);
			}
			
			return blobOffset;
		}

		bool
		Read(
			IReader*										aReader,
			size_t*											aOutBlobOffset) override
		{		
			size_t startOffset = aReader->GetTotalBytesRead();

			if(!ReadBase(aReader))
				return false;
			if(!m_key.Read(aReader))
				return false;
			if(!m_meta.Read(aReader))
				return false;
			if(!aReader->ReadUInt(m_lockSeq))
				return false;

			if(!HasTombstone())
			{
				size_t size;
				if (!aReader->ReadUInt<size_t>(size))
					return false;

				if (aOutBlobOffset != NULL)
					(*aOutBlobOffset) += aReader->GetTotalBytesRead() - startOffset;

				std::unique_ptr<IBuffer> blob = std::make_unique<Buffer<1>>();
				blob->SetSize((size_t)size);

				if (aReader->Read(blob->GetPointer(), blob->GetSize()) != blob->GetSize())
					return false;

				m_blob = std::move(blob);

				m_runtimeState.m_storeSize = m_blob->GetSize();
			}

			return true;
		}

		void	
		UpdateBlobBuffer(
			std::unique_ptr<IBuffer>&					aBlobBuffer) noexcept override
		{
			JELLY_ASSERT(m_runtimeState.m_storeSize == aBlobBuffer->GetSize());
			m_blob = std::move(aBlobBuffer);

			m_runtimeState.m_isResident = true;
		}

		size_t	
		GetStoredBlobSize() const noexcept
		{
			return m_runtimeState.m_storeSize;
		}

		// Data access
		const _KeyType&			GetKey() const noexcept { return m_key; }
		const IBuffer*			GetBlob() const noexcept { JELLY_ASSERT(m_blob); return m_blob.get(); }
		IBuffer*				GetBlob() noexcept { JELLY_ASSERT(m_blob); return m_blob.get(); }
		bool					HasBlob() const noexcept { return (bool)m_blob; }
		const _MetaType&		GetMeta() const noexcept { return m_meta; }
		uint32_t				GetLockSeq() const noexcept { return m_lockSeq; }
		const RuntimeState&		GetRuntimeState() const noexcept { return m_runtimeState; }
		RuntimeState&			GetRuntimeState() noexcept { return m_runtimeState; }
	
	private:

		_KeyType							m_key;
		_MetaType							m_meta;
		uint32_t							m_lockSeq;
		std::unique_ptr<IBuffer>			m_blob;

		// Runtime state, not serialized
		RuntimeState						m_runtimeState;
	};

}