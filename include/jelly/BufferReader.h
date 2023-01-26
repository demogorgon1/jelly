#pragma once

#include <jelly/IReader.h>

namespace jelly
{

	/**
	 * \brief IReader implementation for reading from continuous memory. 
	 * 
	 * You can use this to read data you've serialized into the Buffer of a Blob.
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
	 * \see BufferWriter
	 * \see Blob
	 */
	class BufferReader
		: public IReader
	{
	public:
		//! Initializes reader for the specified buffer.
					BufferReader(
						const void*		aBuffer,
						size_t			aBufferSize);
					~BufferReader();
		
		//------------------------------------------------------------------------------
		// IReader implementation
		size_t		Read(
						void*			aBuffer,
						size_t			aBufferSize) override;
		size_t		GetTotalBytesRead() const override;

		//------------------------------------------------------------------------------
		// Data access
		const void* GetCurrentPointer() const { return m_p; }			//!< Returns pointer to current read position.
		size_t		GetRemainingSize() const { return m_remaining; }	//!< Returns number of bytes left after current read position.

	private:

		const uint8_t*	m_p;
		size_t			m_remaining;
		size_t			m_totalBytesRead;
	};

}
