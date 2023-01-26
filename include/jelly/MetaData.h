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
				: m_blobSeq(UINT32_MAX)
				, m_blobNodeIds(UINT32_MAX)
			{
			}

			void
			Write(
				IWriter*			aWriter) const
			{
				JELLY_CHECK(aWriter->WriteUInt(m_blobSeq), "Failed to write lock blob sequence number.");
				JELLY_CHECK(aWriter->WritePOD(m_blobNodeIds), "Failed to write lock time stamp.");
			}

			bool
			Read(
				IReader*			aReader)
			{
				if (!aReader->ReadUInt(m_blobSeq))
					return false;
				if (!aReader->ReadPOD(m_blobNodeIds))
					return false;
				return true;
			}

			// Public data
			uint32_t							m_blobSeq; // UINT32_MAX means not set
			uint32_t							m_blobNodeIds; // 4 uint8_t packed into 1 uint32_t (UINT8_MAX means not set)
		};

	}

}