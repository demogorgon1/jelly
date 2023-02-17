#pragma once

#include "Compression.h"
#include "ErrorUtils.h"
#include "IBuffer.h"
#include "IFileStreamReader.h"
#include "IReader.h"
#include "ItemBase.h"
#include "IStoreWriter.h"
#include "IWriter.h"
#include "LockMetaData.h"

namespace jelly
{

	class WAL;

	// Blob node item
	template <typename _KeyType>
	class BlobNodeItem
		: public ItemBase
	{
	public:
		struct RuntimeState
		{
			RuntimeState()
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
			BlobNodeItem<_KeyType>*				m_next;
			BlobNodeItem<_KeyType>*				m_prev;
		};

		BlobNodeItem(
			const _KeyType&									aKey = _KeyType(),
			uint32_t										aSeq = 0)
			: m_key(aKey)
		{
			SetSeq(aSeq);
		}

		BlobNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			IBuffer*										aBlob)
			: m_key(aKey)
		{
			SetSeq(aSeq);

			m_blob = aBlob;
		}

		void
		SetKey(
			const _KeyType&									aKey)
		{
			m_key = aKey;
		}

		void
		SetBlob(
			IBuffer*										aBlob)
		{
			m_blob.reset(aBlob);
		}

		void
		Reset()
		{
			JELLY_ASSERT(m_runtimeState.m_pendingWAL == NULL);
			JELLY_ASSERT(m_runtimeState.m_next == NULL);
			JELLY_ASSERT(m_runtimeState.m_prev == NULL);
			JELLY_ASSERT(m_runtimeState.m_walInstanceCount == 0);
			JELLY_ASSERT(!m_runtimeState.m_isResident);

			m_key = _KeyType();

			m_runtimeState.m_storeId = 0;
			m_runtimeState.m_storeOffset = 0;
			m_runtimeState.m_storeSize = 0;

			ResetBase();
		}

		void
		MoveFrom(
			BlobNodeItem*									aOther) 
		{
			m_blob = std::move(aOther->m_blob);

			m_key = aOther->m_key;

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
			size_t											aNewStoreOffset)
		{
			m_runtimeState.m_storeId = aNewStoreId;
			m_runtimeState.m_storeOffset = aNewStoreOffset;
		}

		BlobNodeItem<_KeyType>*
		GetNext()
		{
			return m_runtimeState.m_next;
		}

		BlobNodeItem<_KeyType>*
		GetPrev()
		{
			return m_runtimeState.m_prev;
		}

		void
		SetNext(
			BlobNodeItem<_KeyType>*							aNext)
		{
			m_runtimeState.m_next = aNext;
		}

		void
		SetPrev(
			BlobNodeItem<_KeyType>*							aPrev)
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

			size_t blobOffset = 0; 

			if(!HasTombstone())
			{
				JELLY_ASSERT(m_blob);

				JELLY_CHECK(aWriter->WriteUInt<size_t>(m_blob->GetSize()), "Failed to write blob item blob size.");

				blobOffset = aWriter->GetTotalBytesWritten() - startOffset;

				JELLY_CHECK(aWriter->Write(m_blob->GetPointer(), m_blob->GetSize()) == m_blob->GetSize(), "Failed to write blob item blob.");

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
			std::unique_ptr<IBuffer>&					aBlobBuffer) override
		{
			JELLY_ASSERT(m_runtimeState.m_storeSize == aBlobBuffer->GetSize());
			m_blob = std::move(aBlobBuffer);

			m_runtimeState.m_isResident = true;
		}

		size_t	
		GetStoredBlobSize() const
		{
			return m_runtimeState.m_storeSize;
		}

		// Data access
		const _KeyType&			GetKey() const { return m_key; }
		const IBuffer*			GetBlob() const { JELLY_ASSERT(m_blob); return m_blob.get(); }
		IBuffer*				GetBlob() { JELLY_ASSERT(m_blob); return m_blob.get(); }
		bool					HasBlob() const { return (bool)m_blob; }
		const RuntimeState&		GetRuntimeState() const { return m_runtimeState; }
		RuntimeState&			GetRuntimeState() { return m_runtimeState; }
	
	private:

		_KeyType							m_key;
		std::unique_ptr<IBuffer>			m_blob;

		// Runtime state, not serialized
		RuntimeState						m_runtimeState;
	};

}