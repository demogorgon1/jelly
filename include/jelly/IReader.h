#pragma once

#include <stddef.h>

namespace jelly
{

	// Abstract binary reader interface
	class IReader
	{
	public:
		virtual			~IReader() {}

		template <typename _T>
		bool
		ReadUInt(
			_T&						aOutValue)
		{
			// FIXME: varsize int
			return Read(&aOutValue, sizeof(aOutValue)) == sizeof(aOutValue);
		}

		// Virtual interface
		virtual size_t	Read(
							void*	aBuffer,
							size_t	aBufferSize) = 0;
	};

}