#pragma once

namespace jelly
{

	// Generic fixed-size ring-buffer
	template <typename _T>
	class RingBuffer
	{
	public:
		RingBuffer(
			size_t			aSize)
			: m_empty(true)
			, m_writeOffset(0)
		{
			m_buffer.resize(aSize);
		}

		~RingBuffer()
		{

		}

		void
		Add(
			_T				aValue) noexcept
		{
			if(!m_empty)
			{
				// Add to buffer
				JELLY_ASSERT(m_buffer.size() > 0);
				JELLY_ASSERT(m_writeOffset < m_buffer.size());
				m_buffer[m_writeOffset] = aValue;
				m_writeOffset = (m_writeOffset + 1) % m_buffer.size();
			}
			else
			{
				// Buffer is empty, populate it completely with first sample
				for(_T& t : m_buffer)
					t = aValue;

				m_empty = false;
			}
		}

		_T
		GetOldestValue() const noexcept
		{
			// The value following the write offset must be the oldest one
			JELLY_ASSERT(m_buffer.size() > 0);
			return m_buffer[(m_writeOffset + 1) % m_buffer.size()];
		}

	private:

		std::vector<_T>		m_buffer;
		size_t				m_writeOffset;
		bool				m_empty;
	};

}