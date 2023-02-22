#pragma once

#include "ErrorUtils.h"

namespace jelly
{
	
	/**
	 * \brief Template for unsigned integer vector keys.
	 */
	template <typename _T, size_t _Size>
	struct UIntVectorKey
	{
		UIntVectorKey()
		{
			memset(m_values, 0, sizeof(m_values));
		}

		UIntVectorKey(
			const std::vector<_T>&		aValues) 
		{
			JELLY_ASSERT(aValues.size() == _Size);
			memcpy(m_values, &aValues[0], sizeof(m_values));
		}

		UIntVectorKey(
			std::initializer_list<_T>	aValues) 
		{
			JELLY_ASSERT(aValues.size() == _Size);
			memcpy(m_values, aValues.begin(), sizeof(m_values));
		}

		bool		operator==(const UIntVectorKey<_T, _Size>& aOther) const { return memcmp(m_values, aOther.m_values, sizeof(m_values)) == 0; }
		bool		operator!=(const UIntVectorKey<_T, _Size>& aOther) const { return memcmp(m_values, aOther.m_values, sizeof(m_values)) != 0; }
		bool		operator<(const UIntVectorKey<_T, _Size>& aOther) const { return memcmp(m_values, aOther.m_values, sizeof(m_values)) < 0; }
		bool		operator==(const std::vector<_T>& aOther) const { JELLY_ASSERT(aOther.size() == _Size); return memcmp(m_values, &aOther[0], sizeof(m_values)) == 0; }
		bool		operator==(std::initializer_list<_T> aOther) const { JELLY_ASSERT(aOther.size() == _Size); return memcmp(m_values, aOther.begin(), sizeof(m_values)) == 0; }

		void		
		Write(
			IWriter*					aWriter) const 
		{ 
			for(size_t i = 0; i < _Size; i++)
				JELLY_CHECK(aWriter->WriteUInt(m_values[i]), "Failed to write uint vector key."); 
		}
		
		bool		
		Read(
			IReader*					aReader) 
		{ 
			for (size_t i = 0; i < _Size; i++)
			{
				if(!aReader->ReadUInt(m_values[i]))
					return false;
			}
			return true;
		}

		uint64_t
		GetHash() const
		{
			// xor splitmix64'd elements together
			uint64_t hash = 0;
			for(size_t i = 0; i < _Size; i++)
			{
				uint64_t x = (m_values[i] ^ (m_values[i] >> 30ULL)) * 0xbf58476d1ce4e5b9ULL;
				x = (x ^ (x >> 27ULL)) * 0x94d049bb133111ebULL;
				x = x ^ (x >> 31ULL);
				hash ^= x;
			}
			return hash;
		}
		
		_T			m_values[_Size];	///< Key vector values
	};

}