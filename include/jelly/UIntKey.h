#pragma once

#include "ErrorUtils.h"

namespace jelly
{
	
	/**
	 * Template for unsigned integer keys.
	 */
	template <typename _T>
	struct UIntKey
	{
		/**
		 * Hasher object for unsigned integer key.
		 */
		struct Hasher
		{
			std::size_t
			operator()(
				const UIntKey<_T>& aKey) const
			{
				return std::hash<_T>{}(aKey.m_value);
			}
		};

		UIntKey(_T aValue = 0) : m_value(aValue) {}

		void	Write(IWriter* aWriter) const { JELLY_CHECK(aWriter->WriteUInt(m_value), "Failed to write uint key."); }
		bool	Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool	operator==(const UIntKey<_T>& aOther) const { return m_value == aOther.m_value; }
		bool	operator!=(const UIntKey<_T>& aOther) const { return m_value != aOther.m_value; }
		bool	operator<(const UIntKey<_T>& aOther) const { return m_value < aOther.m_value; }
		bool	operator==(const _T& aOther) const { return m_value == aOther; }
		bool	operator!=(const _T& aOther) const { return m_value != aOther; }
		bool	operator<(const _T& aOther) const { return m_value < aOther; }
		
		operator _T() const
		{
			return m_value;
		}
		
		_T		m_value;	///< Key value
	};

}