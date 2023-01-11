#pragma once

#include "IWriter.h"

namespace jelly
{

	template <size_t _StaticSize>
	class BufferWriter
		: public IWriter
	{
	public:
		static_assert(_StaticSize > 0);

		BufferWriter()
			: m_buffer(m_static)
			, m_bufferSize(_StaticSize)
			, m_size(0)
		{

		}

		~BufferWriter()
		{
			if(!IsStatic())
				delete [] m_buffer;
		}

		bool
		IsStatic() const
		{
			return m_static == m_buffer;
		}

		const void*
		GetBuffer() const
		{
			return m_buffer;
		}

		size_t
		GetBufferSize() const
		{
			return m_size; // Return data size, not allocation size
		}
		
		// IWriter implementation
		size_t	
		Write(
			const void*		aBuffer,
			size_t			aBufferSize) override
		{
			_Reserve(m_size + aBufferSize);

			memcpy(m_buffer + m_size, aBuffer, aBufferSize);
			m_size += aBufferSize;

			return aBufferSize;
		}

	private:

		uint8_t			m_static[_StaticSize];
		uint8_t*		m_buffer;
		size_t			m_bufferSize;
		size_t			m_size;

		void
		_Reserve(
			size_t			aSize)
		{
			if(aSize > m_bufferSize)
			{
				uint8_t* newBuffer = new uint8_t[m_bufferSize + _StaticSize];
				
				if(m_bufferSize > 0)
					memcpy(newBuffer, m_buffer, m_bufferSize);

				memset(newBuffer + m_bufferSize, 0, aSize - m_bufferSize);

				m_buffer = newBuffer;
				m_bufferSize += _StaticSize;
			}
		}
	};

}
