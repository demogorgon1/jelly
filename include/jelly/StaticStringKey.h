#pragma once

#include "ErrorUtils.h"

namespace jelly
{
	
	/**
	 * \brief Template for statically sized string keys. Before assigning this key type you must verify that 
	 *        the length doesn't exceed _Size minus one.
	 */
	template <size_t _Size>
	struct StaticStringKey
	{
		static void
		Validate(
			const char*		aValue)
		{
			size_t length = strlen(aValue);
			JELLY_CHECK(length < _Size, Exception::ERROR_STRING_KEY_TOO_LONG, "Key=%s;MaxLength=%zu", aValue, _Size - 1);
		}

		StaticStringKey(
			const char*		aValue = NULL) noexcept 
		{
			if(aValue != NULL)
			{
				m_length = strlen(aValue);
				JELLY_ALWAYS_ASSERT(m_length < _Size);
				memcpy(m_value, aValue, m_length);
				memset(m_value + length, 0, _Size - m_length);
			}
			else
			{				
				m_length = 0;
				memset(m_value, 0, _Size);
			}
		}

		void		
		Write(
			IWriter*		aWriter) const 
		{ 
			JELLY_ASSERT(m_length < _Size);
			aWriter->WriteUInt(m_length); 
			aWriter->Write(m_value, m_length);
		}
		
		bool		
		Read(
			IReader*		aReader) 
		{ 
			if(!aReader->ReadUInt(m_length))
				return false;
			
			if(m_length >= _Size)
				return false;

			return aReader->Read(m_value, m_length); 
		}

		bool		operator==(const StaticStringKey<_Size>& aOther) const noexcept { return m_length == aOther.m_length && memcmp(m_value, aOther.m_value, m_length) == 0; }
		bool		operator!=(const StaticStringKey<_Size>& aOther) const noexcept { return m_length != aOther.m_length || memcmp(m_value, aOther.m_value, m_length) != 0; }
		bool		operator<(const StaticStringKey<_Size>& aOther) const noexcept { return strcmp(m_value, aOther.m_value) < 0; }
		
		uint64_t
		GetHash() const noexcept
		{
			// fnv1a64
			uint64_t x = 14695981039346656037ULL;
			for(size_t i = 0; i < m_length; i++)
			{
				x ^= (uint64_t)m_value[i];
				x *= 1099511628211ULL;
			}
			return x;
		}
		
		size_t		m_length = 0;
		char		m_value[_Size];	///< Key value
	};

}