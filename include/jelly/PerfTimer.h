#pragma once

namespace jelly
{

	// High-resolution timer for performance-related time measurements.
	class PerfTimer
	{
	public:
		PerfTimer()
		{
			m_startTime = std::chrono::high_resolution_clock::now();
		}

		~PerfTimer()
		{

		}

		uint64_t
		GetElapsedMilliseconds()
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
			return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime).count();
		}

		uint64_t
		GetElapsedMicroseconds()
		{
			std::chrono::time_point<std::chrono::high_resolution_clock> currentTime = std::chrono::high_resolution_clock::now();
			return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_startTime).count();
		}
		
	private:

		std::chrono::time_point<std::chrono::high_resolution_clock>		m_startTime;
	};

}