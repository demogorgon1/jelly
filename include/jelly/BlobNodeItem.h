#pragma once

#include "BlobBuffer.h"
#include "Compression.h"
#include "ErrorUtils.h"
#include "IFileStreamReader.h"
#include "IItem.h"
#include "IReader.h"
#include "IStoreWriter.h"
#include "IWriter.h"
#include "MetaData.h"

namespace jelly
{

	class WAL;

	// Blob node item
	template <typename _KeyType, typename _BlobType>
	struct BlobNodeItem
		: public IItem
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
			m_meta.m_seq = aSeq;
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
			m_meta.m_seq = aSeq;
			m_blob = aBlob;
		}

		void
		MoveFrom(
			BlobNodeItem*									aOther) 
		{
			m_blob = std::move(aOther->m_blob);

			m_key = aOther->m_key;
			m_meta.m_seq = aOther->m_meta.m_seq;
			m_meta.m_timeStamp = aOther->m_meta.m_timeStamp;
			m_tombstone = aOther->m_tombstone;
			m_storeId = aOther->m_storeId;
			m_storeOffset = aOther->m_storeOffset;
			m_storeSize = aOther->m_storeSize;
			m_isResident = aOther->m_isResident;
		}

		bool
		Compare(
			const BlobNodeItem*								aOther) const
		{	
			JELLY_ASSERT(m_blob);
			JELLY_ASSERT(aOther->m_blob);

			// Note: no comparison of timestamp
			return m_key == aOther->m_key && *m_blob == *aOther->m_blob && m_meta.m_seq == aOther->m_meta.m_seq && m_tombstone == aOther->m_tombstone;
		}

		bool
		CompactionRead(
			IFileStreamReader*								aStoreReader)
		{
			m_storeOffset = aStoreReader->GetReadOffset();

			return Read(aStoreReader, &m_storeOffset);
		}

		uint64_t
		CompactionWrite(
			uint32_t										aOldestStoreId,
			IStoreWriter*									aStoreWriter)
		{
			m_storeOffset = UINT64_MAX;

			if (!m_tombstone.ShouldPrune(aOldestStoreId))
				m_storeOffset = aStoreWriter->WriteItem(this);

			return m_storeOffset;
		}

		// IItem implementation
		size_t
		Write(
			IWriter*										aWriter) const override
		{
			size_t blobOffset = 0;

			if(!m_tombstone.IsSet())
			{
				JELLY_ASSERT(m_blob);

				VarSizeUInt::Encoder<size_t> blobSize;
				blobSize.Encode(m_blob->GetSize());
				JELLY_CHECK(aWriter->Write(blobSize.GetBuffer(), blobSize.GetBufferSize()) == blobSize.GetBufferSize(), "Failed to write blob item blob size.");
				JELLY_CHECK(aWriter->Write(m_blob->GetPointer(), m_blob->GetSize()) == m_blob->GetSize(), "Failed to write blob item blob.");

				blobOffset = blobSize.GetBufferSize();
			}
			else
			{
				JELLY_ASSERT(!m_blob);
			}
			
			m_key.Write(aWriter);
			m_meta.Write(aWriter);
			m_tombstone.Write(aWriter);

			return blobOffset;
		}

		bool
		Read(
			IReader*										aReader,
			size_t*											aOutBlobOffset) override
		{		
			size_t size;
			size_t sizeSize;
			if(!aReader->ReadUInt<size_t>(size, &sizeSize))
				return false;

			if(aOutBlobOffset != NULL)
				(*aOutBlobOffset) += sizeSize;

			std::unique_ptr<BlobBuffer> blob = std::make_unique<BlobBuffer>();
			blob->SetSize((size_t)size);

			if(aReader->Read(blob->GetPointer(), blob->GetSize()) != blob->GetSize())
				return false;

			if (!m_key.Read(aReader))
				return false;
			if(!m_meta.Read(aReader))
				return false;
			if(!m_tombstone.Read(aReader))
				return false;

			m_blob = std::move(blob);
			m_storeSize = m_blob->GetSize();
			m_isResident = true;

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
		MetaData::Blob						m_meta;
		MetaData::Tombstone					m_tombstone;
	
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