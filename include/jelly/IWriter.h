#pragma once

namespace jelly
{

	// Abstract binary writer interface
	class IWriter
	{
	public:
		virtual			~IWriter() {}

		// Virtual interface
		virtual size_t	Write(
							const void*	aBuffer,
							size_t		aBufferSize) = 0;
	};

}