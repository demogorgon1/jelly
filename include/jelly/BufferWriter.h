#pragma once

#include "IBuffer.h"
#include "IWriter.h"

namespace jelly
{

	/**
	 * \brief IWriter implementation for reading into a IBuffer. 
	 * 
	 * The buffer will automatically grow as you add more data. 
	 *
	 * \code
	 * // Write something into a buffer
	 * Buffer<1> buffer;
	 * BufferWriter writer(&buffer);
	 * writer.WriteUInt<uint32_t>(1234);
	 *
	 * // Read it back
	 * BufferReader reader(buffer.GetPointer(), buffer.GetSize());
	 * uint32_t something = reader.ReadUInt<uint32_t>();
	 * JELLY_ASSERT(something == 1234);
	 * \endcode
	 * 
	 * \see Buffer
	 * \see BufferReader
	 * \see Blob
	 */
	class BufferWriter
		: public IWriter
	{
	public:
		BufferWriter(
			IBuffer&	aBuffer) noexcept
			: m_buffer(aBuffer) 
		{

		}

		~BufferWriter()
		{
		}

		//------------------------------------------------------------------------------
		// IWriter implementation
		void		
		Write(
			const void*		aBuffer,
			size_t			aBufferSize) override
		{
			size_t writeOffset = m_buffer.GetSize();

			m_buffer.SetSize(writeOffset + aBufferSize);

			memcpy((uint8_t*)m_buffer.GetPointer() + writeOffset, aBuffer, aBufferSize);
		}

		size_t
		GetTotalBytesWritten() const noexcept override
		{
			return m_buffer.GetSize();
		}

	private:

		IBuffer&	m_buffer;
	};

}
