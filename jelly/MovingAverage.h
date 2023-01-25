#pragma once

#include <jelly/ErrorUtils.h>

namespace jelly
{

	// Fixed-size moving average filter
	template <typename _T>
	class MovingAverage
	{
	public:
		MovingAverage(
			size_t			aSize)
			: m_i(0)
			, m_sum(0)
		{
			m_buffer.resize(aSize, _T(0));
		}
		
		~MovingAverage()
		{

		}
		
		_T
		Update(
			const _T&		aValue)
		{
			JELLY_ASSERT(m_sum >= m_buffer[m_i]);
			JELLY_ASSERT(m_buffer.size() > 0);
			m_sum -= m_buffer[m_i];
			m_sum += aValue;
			m_buffer[m_i] = aValue;		
			m_i = (m_i + 1) % m_buffer.size();
			return m_sum / _T(m_buffer.size());
		}

	private:

		_T					m_sum;
		std::vector<_T>		m_buffer;
		size_t				m_i;
	};

}