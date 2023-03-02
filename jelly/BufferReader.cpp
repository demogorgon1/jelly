#include <jelly/Base.h>

#include <jelly/BufferReader.h>

namespace jelly
{

	BufferReader::BufferReader(
		const void*		aBuffer,
		size_t			aBufferSize) noexcept
		: m_p((const uint8_t*)aBuffer)
		, m_remaining(aBufferSize)
		, m_totalBytesRead(0)
	{

	}
		
	BufferReader::~BufferReader()
	{

	}

	//-----------------------------------------------------------------------------

	size_t	
	BufferReader::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		size_t toCopy = std::min<size_t>(aBufferSize, m_remaining);
			
		if(toCopy > 0)
		{
			memcpy(aBuffer, m_p, toCopy);
			m_p += toCopy;
			m_remaining -= toCopy;
		}

		m_totalBytesRead += toCopy;

		return toCopy;
	}

	size_t		
	BufferReader::GetTotalBytesRead() const 
	{
		return m_totalBytesRead;
	}

}