#pragma once

namespace jelly
{

	struct FileHeader
	{
		enum Type : uint8_t
		{
			TYPE_NONE,
			TYPE_WAL,
			TYPE_STORE
		};

		enum Flag : uint8_t
		{
			FLAG_ITEM_COMPRESSION = 0x01,
			FLAG_STREAM_COMPRESSION = 0x02,
			FLAG_STORE_ORIGIN = 0x04		// Header is followed by the store's origin id (uint32_t), see IHost::StoreInfo
		};

		FileHeader(
			Type						aType = Type(0),
			uint8_t						aFlags = 0,
			uint8_t						aCompressionId = 0) noexcept
			: m_type(aType)
			, m_flags(aFlags)
			, m_compressionId(aCompressionId)
		{

		}

		bool
		operator==(
			const FileHeader&			aOther) const noexcept
		{
			return m_type == aOther.m_type && m_flags == aOther.m_flags && m_compressionId == aOther.m_compressionId;
		}

		// Returns true if a file with this (expected) header can read a file with the specified header. Stores can have
		// an origin or not (stores written before origins were added don't).
		bool
		Accepts(
			const FileHeader&			aActual) const noexcept
		{
			return m_type == aActual.m_type &&
				(m_flags & ~FLAG_STORE_ORIGIN) == (aActual.m_flags & ~FLAG_STORE_ORIGIN) &&
				m_compressionId == aActual.m_compressionId;
		}

		// Public data
		Type		m_type;
		uint8_t		m_flags;
		uint8_t		m_compressionId;
	};

}