#pragma once

namespace jelly
{

	/**
	 * \brief Template for unsigned integer locks.
	 */
	template <typename _T>
	struct UIntLock
	{
		UIntLock(_T aValue = 0) : m_value(aValue) {}

		void	Write(IWriter* aWriter) const { JELLY_CHECK(aWriter->WriteUInt(m_value), "Failed to write uint lock."); }
		bool	Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool	operator==(const UIntLock& aOther) const { return m_value == aOther.m_value; }
		bool	operator!=(const UIntLock& aOther) const { return m_value != aOther.m_value; }
		bool	operator==(const _T& aOther) const { return m_value == aOther; }
		bool	operator!=(const _T& aOther) const { return m_value != aOther; }
		bool	IsSet() const { return m_value != 0; }
		void	Clear() { m_value = 0; }

		_T	m_value; ///< Lock value
	};

}

