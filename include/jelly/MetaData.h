#pragma once

#include "ErrorUtils.h"
#include "IReader.h"
#include "IWriter.h"

namespace jelly
{

	// Item meta data as stored on disk (both stores and WALs)
	namespace MetaData
	{

		struct Lock
		{
			Lock()
				: m_seq(0)
				, m_timeStamp(0)
				, m_blobSeq(UINT32_MAX)
				, m_blobNodeIds(UINT32_MAX)
			{
			}

			void
			Write(
				IWriter*			aWriter) const
			{
				JELLY_CHECK(aWriter->WriteUInt(m_timeStamp), "Failed to write lock time stamp.");
				JELLY_CHECK(aWriter->WriteUInt(m_seq), "Failed to write lock sequence number.");
				JELLY_CHECK(aWriter->WriteUInt(m_blobSeq), "Failed to write lock blob sequence number.");
				JELLY_CHECK(aWriter->WritePOD(m_blobNodeIds), "Failed to write lock time stamp.");
			}

			bool
			Read(
				IReader*			aReader)
			{
				if (!aReader->ReadUInt(m_timeStamp))
					return false;
				if (!aReader->ReadUInt(m_seq))
					return false;
				if (!aReader->ReadUInt(m_blobSeq))
					return false;
				if (!aReader->ReadPOD(m_blobNodeIds))
					return false;
				return true;
			}

			// Public data
			uint64_t							m_timeStamp;
			uint32_t							m_seq;
			uint32_t							m_blobSeq; // UINT32_MAX means not set
			uint32_t							m_blobNodeIds; // 4 uint8_t packed into 1 uint32_t (UINT8_MAX means not set)
		};

		struct Blob
		{
			Blob()
				: m_seq(0)
				, m_timeStamp(0)
			{
			}

			void
			Write(
				IWriter*			aWriter) const
			{
				JELLY_CHECK(aWriter->WriteUInt(m_timeStamp), "Failed to write blob time stamp.");
				JELLY_CHECK(aWriter->WriteUInt(m_seq), "Failed to write blob sequence number.");
			}

			bool
			Read(
				IReader*			aReader)
			{
				if (!aReader->ReadUInt(m_timeStamp))
					return false;
				if (!aReader->ReadUInt(m_seq))
					return false;
				return true;
			}

			// Public data
			uint64_t							m_timeStamp;
			uint32_t							m_seq;
		};

		struct Tombstone
		{
			Tombstone()
				: m_storeId(UINT32_MAX)
			{

			}

			void
			Write(
				IWriter*			aWriter) const
			{
				JELLY_CHECK(aWriter->WritePOD(m_storeId), "Failed to write tombstone store id.");
			}

			bool
			Read(
				IReader*			aReader)
			{
				if (!aReader->ReadPOD(m_storeId))
					return false;
				return true;
			}

			void
			Clear()
			{
				m_storeId = UINT32_MAX;
			}

			void
			Set(
				uint32_t			aStoreId)
			{
				JELLY_ASSERT(aStoreId != UINT32_MAX);
				m_storeId = aStoreId;
			}

			bool
			ShouldPrune(
				uint32_t			aOldestStoreId) const
			{
				return m_storeId != UINT32_MAX && aOldestStoreId > m_storeId;
			}

			bool
			IsSet() const
			{
				return m_storeId != UINT32_MAX;
			}

			bool
			operator ==(
				const Tombstone&	aOther) const
			{
				return m_storeId == aOther.m_storeId;
			}

			// Public data
			uint32_t							m_storeId;
		};

	}

}