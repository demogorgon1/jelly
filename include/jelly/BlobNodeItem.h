#pragma once

#include "BlobBuffer.h"
#include "Compression.h"
#include "ErrorUtils.h"
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
	template <typename _KeyType, typename _BlobType>
	struct BlobNodeItem
		: public ItemBase
	{
		BlobNodeItem(
			const _KeyType&									aKey = _KeyType(),
			uint32_t										aSeq = 0)
			: m_key(aKey)
			, m_pendingWAL(NULL)
			, m_next(NULL)
			, m_prev(NULL)
			, m_storeId(0)
			, m_storeOffset(0)
			, m_storeSize(0)
			, m_walInstanceCount(0)
			, m_isResident(false)
		{
			SetSeq(aSeq);
		}

		BlobNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			BlobBuffer*										aBlob)
			: m_key(aKey)
			, m_pendingWAL(NULL)
			, m_next(NULL)
			, m_prev(NULL)
			, m_storeId(0)
			, m_storeOffset(0)
			, m_storeSize(0)
			, m_walInstanceCount(0)
			, m_isResident(false)
		{
			SetSeq(aSeq);

			m_blob = aBlob;
		}

		void
		Reset()
		{
			JELLY_ASSERT(m_pendingWAL == NULL);
			JELLY_ASSERT(m_next == NULL);
			JELLY_ASSERT(m_prev == NULL);
			JELLY_ASSERT(m_walInstanceCount == 0);
			JELLY_ASSERT(!m_isResident);

			m_key = _KeyType();
			m_storeId = 0;
			m_storeOffset = 0;
			m_storeSize = 0;			

			ResetBase();
		}

		void
		MoveFrom(
			BlobNodeItem*									aOther) 
		{
			m_blob = std::move(aOther->m_blob);

			m_key = aOther->m_key;
			m_storeId = aOther->m_storeId;
			m_storeOffset = aOther->m_storeOffset;
			m_storeSize = aOther->m_storeSize;
			m_isResident = aOther->m_isResident;

			CopyBase(*aOther);
		}

		bool
		Compare(
			const BlobNodeItem*								aOther) const
		{	
			JELLY_ASSERT(m_blob);
			JELLY_ASSERT(aOther->m_blob);

			// Note: no comparison of timestamp
			return m_key == aOther->m_key && *m_blob == *aOther->m_blob && CompareBase(*aOther);
		}

		bool
		CompactionRead(
			IFileStreamReader*								aStoreReader)
		{
			m_storeOffset = aStoreReader->GetTotalBytesRead();

			return Read(aStoreReader, &m_storeOffset);
		}

		uint64_t
		CompactionWrite(
			uint32_t										aOldestStoreId,
			IStoreWriter*									aStoreWriter)
		{
			m_storeOffset = UINT64_MAX;

			if (!ShouldBePruned(aOldestStoreId))
				m_storeOffset = aStoreWriter->WriteItem(this);

			return m_storeOffset;
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

				std::unique_ptr<BlobBuffer> blob = std::make_unique<BlobBuffer>();
				blob->SetSize((size_t)size);

				if (aReader->Read(blob->GetPointer(), blob->GetSize()) != blob->GetSize())
					return false;

				m_blob = std::move(blob);
				m_storeSize = m_blob->GetSize();
			}

			return true;
		}

		void	
		UpdateBlobBuffer(
			std::unique_ptr<BlobBuffer>&					aBlobBuffer) override
		{
			JELLY_ASSERT(m_storeSize == aBlobBuffer->GetSize());
			m_blob = std::move(aBlobBuffer);
			m_isResident = true;
		}

		size_t	
		GetStoredBlobSize() const
		{
			return m_storeSize;
		}

		// Public data
		_KeyType							m_key;
		std::unique_ptr<BlobBuffer>			m_blob;
	
		// Remaining members are part of runtime state, not serialized
		WAL*								m_pendingWAL;
		uint32_t							m_walInstanceCount;
		uint32_t							m_storeId;
		size_t								m_storeOffset;
		size_t								m_storeSize;
		bool								m_isResident;
		
		// Placement in timestamp-sorted linked list used to expell oldest items when memory limit is reached
		BlobNodeItem<_KeyType, _BlobType>*	m_next; 
		BlobNodeItem<_KeyType, _BlobType>*	m_prev;
	};

}