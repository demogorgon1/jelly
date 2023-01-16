#pragma once

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
		size_t		Read(
						void*			aBuffer,
						size_t			aBufferSize) override;

		// Data access
		const void* GetCurrentPointer() const { return m_p; }
		size_t		GetRemainingSize() const { return m_remaining; }


	private:

		const uint8_t*	m_p;
		size_t			m_remaining;
	};

}
