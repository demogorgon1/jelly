#pragma once

namespace jelly
{

	template <typename _T>
	struct UIntLock
	{
		UIntLock(_T aValue = 0) : m_value(aValue) {}

		bool	Write(IWriter* aWriter) const { return aWriter->WriteUInt(m_value); }
		bool	Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool	operator==(const UIntLock& aOther) const { return m_value == aOther.m_value; }
		bool	operator!=(const UIntLock& aOther) const { return m_value != aOther.m_value; }
		bool	IsSet() const { return m_value != 0; }
		void	Clear() { m_value = 0; }

		_T	m_value;
	};

}
