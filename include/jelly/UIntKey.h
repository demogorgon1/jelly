#pragma once

#include "ErrorUtils.h"

namespace jelly
{
	
	/**
	 * \brief Template for unsigned integer keys.
	 */
	template <typename _T>
	struct UIntKey
	{
		UIntKey(_T aValue = _T(0)) : m_value(aValue) {}

		void		Write(IWriter* aWriter) const { JELLY_CHECK(aWriter->WriteUInt(m_value), "Failed to write uint key."); }
		bool		Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool		operator==(const UIntKey<_T>& aOther) const { return m_value == aOther.m_value; }
		bool		operator!=(const UIntKey<_T>& aOther) const { return m_value != aOther.m_value; }
		bool		operator<(const UIntKey<_T>& aOther) const { return m_value < aOther.m_value; }
		bool		operator==(const _T& aOther) const { return m_value == aOther; }
		bool		operator!=(const _T& aOther) const { return m_value != aOther; }
		bool		operator<(const _T& aOther) const { return m_value < aOther; }
		
		operator _T() const
		{
			return m_value;
		}

		uint64_t
		GetHash() const
		{
			// splitmix64
			uint64_t x = (m_value ^ (m_value >> 30ULL)) * 0xbf58476d1ce4e5b9ULL;
			x = (x ^ (x >> 27ULL)) * 0x94d049bb133111ebULL;
			x = x ^ (x >> 31ULL);
			return x;
		}
		
		_T			m_value;	///< Key value
	};

}