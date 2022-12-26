#pragma once

#include <assert.h>

#include "Compression.h"
#include "IItem.h"
#include "IReader.h"
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
		{
			m_meta.m_seq = aSeq;
		}

		BlobNodeItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			const _BlobType&								aBlob)
			: m_key(aKey)
			, m_pendingWAL(NULL)
			, m_next(NULL)
			, m_prev(NULL)
			, m_storeId(0)
			, m_storeOffset(0)
		{
			m_meta.m_seq = aSeq;
			m_blob.Copy(aBlob);
		}

		void
		CopyFrom(
			BlobNodeItem*									aOther) 
		{
			m_key = aOther->m_key;
			m_blob.Copy(aOther->m_blob);
			m_meta.m_seq = aOther->m_meta.m_seq;
			m_meta.m_timeStamp = aOther->m_meta.m_timeStamp;
			m_storeId = aOther->m_storeId;
			m_storeOffset = aOther->m_storeOffset;
		}

		bool
		Compare(
			const BlobNodeItem*								aOther) const
		{	
			// Note: no comparison of timestamp
			return m_key == aOther->m_key && m_blob == aOther->m_blob && m_meta.m_seq == aOther->m_meta.m_seq;
		}

		// IItem implementation
		bool
		Write(
			IWriter*										aWriter,
			const Compression::IProvider*					aItemCompression) const override
		{
			assert(m_blob.IsSet());

			if(!m_blob.Write(aWriter, aItemCompression))
				return false;
			if(!m_key.Write(aWriter))
				return false;
			if(aWriter->Write(&m_meta, sizeof(m_meta)) != sizeof(m_meta))
				return false;
			return true;
		}

		bool
		Read(
			IReader*										aReader,
			const Compression::IProvider*					aItemCompression,
			ReadType										aReadType = READ_TYPE_ALL) override
		{		
			if (!m_blob.Read(aReader, aItemCompression))
				return false;

			if(aReadType != READ_TYPE_BLOB_ONLY)
			{
				assert(aReadType == READ_TYPE_ALL);

				if (!m_key.Read(aReader))
					return false;
				if (aReader->Read(&m_meta, sizeof(m_meta)) != sizeof(m_meta))
					return false;
			}

			return true;
		}

		size_t	
		GetStoredBlobSize() const
		{
			return m_blob.GetStoredSize();
		}

		// Public data
		_KeyType							m_key;
		_BlobType							m_blob;	
		MetaData::Blob						m_meta;
	
		// Remaining members are part of runtime state, not serialized
		WAL*								m_pendingWAL;
		uint32_t							m_storeId;
		size_t								m_storeOffset;

		// Placement in timestamp-sorted linked list used to expell oldest items when memory limit is reached
		BlobNodeItem<_KeyType, _BlobType>*	m_next; 
		BlobNodeItem<_KeyType, _BlobType>*	m_prev;
	};

}