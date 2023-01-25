#pragma once

#include "VarSizeUInt.h"

namespace jelly
{

	/**
	 * \brief Abstract binary writer interface.
	 *
	 * \see IReader
	 */
	class IWriter
	{
	public:
		virtual			~IWriter() {}

		//! Writes a variably sized unsigned integer.
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

		//! Writes a POD type.
		template <typename _T>
		bool
		WritePOD(
			const _T&					aValue)
		{
			return Write(&aValue, sizeof(_T)) == sizeof(_T);
		}

		//------------------------------------------------------------------------------------
		// Virtual interface

		//! Writes a buffer. Returns number of bytes written.
		virtual size_t	Write(
							const void*	aBuffer,
							size_t		aBufferSize) = 0;
	};

}