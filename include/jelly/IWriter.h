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
		void
		WriteUInt(
			_T							aValue)
		{
			#if defined(JELLY_VAR_SIZE_UINTS)
				VarSizeUInt::Encoder<_T> t;
				t.Encode(aValue);
				Write(t.GetBuffer(), t.GetBufferSize());
			#else
				WritePOD(aValue);
			#endif
		}

		//! Writes a POD type.
		template <typename _T>
		void
		WritePOD(
			const _T&					aValue)
		{
			Write(&aValue, sizeof(_T));
		}

		//------------------------------------------------------------------------------------
		// Virtual interface

		//! Writes a buffer.
		virtual void	Write(
							const void*	aBuffer,
							size_t		aBufferSize) = 0;

		//! Returns number of bytes written in total.
		virtual size_t	GetTotalBytesWritten() const = 0;
	};

}