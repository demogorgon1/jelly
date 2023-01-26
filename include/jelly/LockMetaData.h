#pragma once

#include "ErrorUtils.h"
#include "IReader.h"
#include "IWriter.h"

namespace jelly
{

	namespace LockMetaData
	{

		template <size_t _MaxBlobNodeIds>
		struct StaticSingleBlob
		{
			StaticSingleBlob()
				: m_blobSeq(UINT32_MAX)
				, m_blobNodeIdCount(0)
			{
			}

			StaticSingleBlob(
				uint32_t						aBlobSeq,
				const std::vector<uint32_t>&	aBlobNodeIds)
				: m_blobSeq(aBlobSeq)
			{
				JELLY_ASSERT(aBlobNodeIds.size() <= _MaxBlobNodeIds);
				m_blobNodeIdCount = (uint32_t)aBlobNodeIds.size();
				for(size_t i = 0; i < m_blobNodeIdCount; i++)
					m_blobNodeIds[i] = aBlobNodeIds[i];
			}

			void
			Write(
				IWriter*						aWriter) const
			{
				JELLY_CHECK(aWriter->WriteUInt(m_blobSeq), "Failed to write lock blob sequence number.");
				JELLY_CHECK(aWriter->WriteUInt(m_blobNodeIdCount), "Failed to write lock blob node id count.");
				for(uint32_t i = 0; i < m_blobNodeIdCount; i++)
					JELLY_CHECK(aWriter->WriteUInt(m_blobNodeIds[i]), "Failed to write lock blob node id.");
			}

			bool
			Read(
				IReader*						aReader)
			{
				if (!aReader->ReadUInt(m_blobSeq))
					return false;
				if (!aReader->ReadUInt(m_blobNodeIdCount))
					return false;

				for(uint32_t i = 0; i < m_blobNodeIdCount; i++)
				{
					if(!aReader->ReadUInt(m_blobNodeIds[i]))
						return false;
				}

				return true;
			}

			// Public data
			uint32_t							m_blobNodeIds[_MaxBlobNodeIds];
			uint32_t							m_blobSeq; 
			uint32_t							m_blobNodeIdCount;
		};

	}

}