#pragma once

namespace jelly
{

	// Abstract binary reader interface
	class IReader
	{
	public:
		virtual			~IReader() {}

		// Virtual interface
		virtual size_t	Read(
							void*	aBuffer,
							size_t	aBufferSize) = 0;
	};

}