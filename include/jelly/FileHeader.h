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
			FLAG_STREAM_COMPRESSION = 0x02
		};

		FileHeader(
			Type						aType = Type(0),
			uint8_t						aFlags = 0,
			uint8_t						aCompressionId = 0)
			: m_type(aType)
			, m_flags(aFlags)
			, m_compressionId(aCompressionId)
		{

		}

		bool
		operator==(
			const FileHeader&			aOther) const
		{
			return m_type == aOther.m_type && m_flags == aOther.m_flags && m_compressionId == aOther.m_compressionId;
		}

		// Public data
		Type		m_type;
		uint8_t		m_flags;
		uint8_t		m_compressionId;
	};

}