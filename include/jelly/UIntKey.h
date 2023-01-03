#pragma once

namespace jelly
{
	
	template <typename _T>
	struct UIntKey
	{
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

		bool	Write(IWriter* aWriter) const { return aWriter->WriteUInt(m_value); }
		bool	Read(IReader* aReader) { return aReader->ReadUInt(m_value); }
		bool	operator==(const UIntKey<_T>& aOther) const { return m_value == aOther.m_value; }
		bool	operator!=(const UIntKey<_T>& aOther) const { return m_value != aOther.m_value; }
		bool	operator<(const UIntKey<_T>& aOther) const { return m_value < aOther.m_value; }

		uint32_t	m_value;
	};

}