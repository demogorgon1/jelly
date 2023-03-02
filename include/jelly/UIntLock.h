#pragma once

namespace jelly
{

	/**
	 * \brief Template for unsigned integer locks. Value of 0 means unassigned.
	 */
	template <typename _T>
	struct UIntLock
	{
		UIntLock(_T aValue = 0) noexcept : m_value(aValue) {}

		void	Write(IWriter* aWriter) const { aWriter->WriteUInt(m_value); }
		bool	Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool	operator==(const UIntLock& aOther) const noexcept { return m_value == aOther.m_value; }
		bool	operator!=(const UIntLock& aOther) const noexcept { return m_value != aOther.m_value; }
		bool	operator==(const _T& aOther) const noexcept { return m_value == aOther; }
		bool	operator!=(const _T& aOther) const noexcept { return m_value != aOther; }
		bool	IsSet() const noexcept { return m_value != 0; }
		void	Clear() noexcept { m_value = 0; }

		operator _T() const noexcept
		{
			return m_value;
		}

		_T	m_value; ///< Lock value
	};

}

