#pragma once

#include <stddef.h>
#include <stdint.h>

#include <jelly/IReader.h>

namespace jelly
{

	class BufferReader
		: public IReader
	{
	public:
				BufferReader(
					const void*		aBuffer,
					size_t			aBufferSize);
				~BufferReader();
		
		// IReader implementation
		size_t	Read(
					void*			aBuffer,
					size_t			aBufferSize) override;

	private:

		const uint8_t*	m_p;
		size_t			m_remaining;
	};

}