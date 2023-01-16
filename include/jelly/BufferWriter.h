#pragma once

#include "IBuffer.h"
#include "IWriter.h"

namespace jelly
{

	class BufferWriter
		: public IWriter
	{
	public:
		BufferWriter(
			IBuffer&	aBuffer)
			: m_buffer(aBuffer)
		{

		}

		~BufferWriter()
		{
		}

		// IWriter implementation
		size_t	
		Write(
			const void*		aBuffer,
			size_t			aBufferSize) override
		{
			size_t writeOffset = m_buffer.GetSize();

			m_buffer.SetSize(writeOffset + aBufferSize);

			memcpy((uint8_t*)m_buffer.GetPointer() + writeOffset, aBuffer, aBufferSize);

			return aBufferSize;
		}

	private:

		IBuffer&	m_buffer;
	};

}
