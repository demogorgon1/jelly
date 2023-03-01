#pragma once

#include "ErrorUtils.h"
#include "IReader.h"
#include "IWriter.h"

namespace jelly
{

	namespace MetaData
	{

		/**
		 * \brief Lock node meta data for when every lock key maps directly to a single blob.
		 */
		template <size_t _MaxBlobNodeIds = 1>
		struct LockStaticSingleBlob
		{
			LockStaticSingleBlob()
				: m_blobSeq(UINT32_MAX)
				, m_blobNodeIdCount(0)
			{
			}

			LockStaticSingleBlob(
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
				aWriter->WriteUInt(m_blobSeq); 
				aWriter->WriteUInt(m_blobNodeIdCount);
				
				for(uint32_t i = 0; i < m_blobNodeIdCount; i++)
					aWriter->WriteUInt(m_blobNodeIds[i]);
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
			uint32_t							m_blobNodeIds[_MaxBlobNodeIds];	//!< Blob node ids where blob is stored.
			uint32_t							m_blobSeq;						//!< Latest blob sequence number.
			uint32_t							m_blobNodeIdCount;				//!< Number of entries in the m_blobNodeIds array.
		};

		/**
		* \brief Dummy meta data.
		*/
		struct Dummy
		{
			void
			Write(
				IWriter*						/*aWriter*/) const
			{

			}

			bool
			Read(
				IReader*						/*aReader*/)
			{
				return true;
			}
		};

		/**
		* \brief Single unsigned integer as meta data.
		*/
		template <typename _T>
		struct UInt
		{
			UInt(
				_T								aValue = 0)
				: m_value(aValue)
			{

			}

			void
			Write(
				IWriter*						aWriter) const
			{
				aWriter->WriteUInt(m_value);
			}

			bool
			Read(
				IReader*						aReader)
			{
				return aReader->ReadUInt(m_value);
			}

			bool
			operator ==(
				_T								aOther) const
			{
				return m_value == aOther;
			}

			// Public data
			_T									m_value;
		};

	}

}