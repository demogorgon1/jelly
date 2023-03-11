#pragma once

#include "ErrorUtils.h"
#include "IBuffer.h"

namespace jelly
{

	class IReader;
	class IWriter;

	// Base class for LockNode and BlobNode items
	class ItemBase
	{
	public:
						ItemBase() noexcept;
		virtual			~ItemBase();

		bool			HasTombstone() const noexcept;
		void			SetTombstoneStoreId(
							uint32_t						aTombstoneStoreId) noexcept;
		void			RemoveTombstone() noexcept;
		bool			ShouldBePruned(
							uint32_t						aCurrentOldestStoreId) const noexcept;
		void			SetTimeStamp(
							uint64_t						aTimeStamp) noexcept;
		void			SetSeq(
							uint32_t						aSeq) noexcept;
		uint32_t		IncrementSeq() noexcept;

		// Virtual interface
		virtual size_t	Write(
							IWriter*						aWriter) const = 0;
		virtual bool	Read(
							IReader*						aReader,
							size_t*							aOutBlobOffset) = 0;

		// Virtual methods
		virtual void	UpdateBlobBuffer(
							std::unique_ptr<IBuffer>&		/*aBlobBuffer*/) { JELLY_ASSERT(false); }
		virtual size_t	GetStoredBlobSize() const { JELLY_ASSERT(false); return 0; }

		// Data access
		uint32_t		GetTombstoneStoreId() const noexcept { return m_data.m_tombstoneStoreId; }
		uint32_t		GetSeq() const noexcept { return m_data.m_seq; }
		uint64_t		GetTimeStamp() const noexcept { return m_data.m_timeStamp; }

	protected:

		void			ResetBase();
		void			WriteBase(
							IWriter*						aWriter) const;
		bool			ReadBase(
							IReader*						aReader);
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