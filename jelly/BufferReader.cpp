#include <algorithm>

#include <string.h>

#include <jelly/BufferReader.h>

namespace jelly
{

	BufferReader::BufferReader(
		const void*		aBuffer,
		size_t			aBufferSize)
		: m_p((const uint8_t*)aBuffer)
		, m_remaining(aBufferSize)
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
		size_t toCopy = std::min(aBufferSize, m_remaining);
			
		if(toCopy > 0)
		{
			memcpy(aBuffer, m_p, toCopy);
			m_p += toCopy;
			m_remaining -= toCopy;
		}

		return toCopy;
	}

}