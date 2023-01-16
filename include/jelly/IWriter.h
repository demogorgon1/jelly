#pragma once

#include "VarSizeUInt.h"

namespace jelly
{

	// Abstract binary writer interface
	class IWriter
	{
	public:
		virtual			~IWriter() {}

		template <typename _T>
		bool
		WriteUInt(
			_T							aValue)
		{
			VarSizeUInt::Encoder<_T> t;
			t.Encode(aValue);
			if(Write(t.GetBuffer(), t.GetBufferSize()) != t.GetBufferSize())
				return false;
			
			return true;
		}

		template <typename _T>
		bool
		WritePOD(
			const _T&					aValue)
		{
			return Write(&aValue, sizeof(_T)) == sizeof(_T);
		}

		// Virtual interface
		virtual size_t	Write(
							const void*	aBuffer,
							size_t		aBufferSize) = 0;
	};

}