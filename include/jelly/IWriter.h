#pragma once

#include <stddef.h>

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
			// FIXME: varsize int
			return Write(&aValue, sizeof(aValue)) == sizeof(aValue);
		}

		// Virtual interface
		virtual size_t	Write(
							const void*	aBuffer,
							size_t		aBufferSize) = 0;
	};

}