#pragma once

#include "BlobBuffer.h"
#include "ErrorUtils.h"

namespace jelly
{

	class IReader;
	class IWriter;

	// Base class for LockNode and BlobNode items
	class ItemBase
	{
	public:
						ItemBase();
		virtual			~ItemBase();

		bool			HasTombstone() const;
		void			SetTombstoneStoreId(
							uint32_t						aTombstoneStoreId);
		void			RemoveTombstone();
		bool			ShouldBePruned(
							uint32_t						aCurrentOldestStoreId) const;
		void			SetTimeStamp(
							uint64_t						aTimeStamp);
		void			SetSeq(
							uint32_t						aSeq);
		void			IncrementSeq();

		// Virtual interface
		virtual size_t	Write(
							IWriter*						aWriter) const = 0;
		virtual bool	Read(
							IReader*						aReader,
							size_t*							aOutBlobOffset) = 0;

		// Virtual methods
		virtual void	UpdateBlobBuffer(
							std::unique_ptr<BlobBuffer>&	/*aBlobBuffer*/) { JELLY_ASSERT(false); }
		virtual size_t	GetStoredBlobSize() const { JELLY_ASSERT(false); return 0; }

		// Data access
		uint32_t		GetTombstoneStoreId() const { return m_data.m_tombstoneStoreId; }
		uint32_t		GetSeq() const { return m_data.m_seq; }
		uint64_t		GetTimeStamp() const { return m_data.m_timeStamp; }

	protected:

		void			ResetBase();
		void			WriteBase(
							IWriter*						aWriter) const;
		bool			ReadBase(
							IReader*						aReader);
		bool			CompareBase(
							const ItemBase&					aItemBase) const;
		void			CopyBase(
							const ItemBase&					aItemBase);

	private:

		struct Data
		{
			Data()
				: m_tombstoneStoreId(UINT32_MAX)
				, m_seq(0)
				, m_timeStamp(0)
			{

			}

			uint64_t		m_timeStamp;
			uint32_t		m_seq;
			uint32_t		m_tombstoneStoreId;
		};

		Data				m_data;
	};

}