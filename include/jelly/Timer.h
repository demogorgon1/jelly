#pragma once

namespace jelly::Test::Sim
{

	class Timer
	{
	public:
		Timer(
			uint32_t		aMilliseconds = 0)
		{
			SetTimeout(aMilliseconds);
		}

		~Timer()
		{

		}

		void
		SetTimeout(
			uint32_t		aMilliseconds)
		{
			m_interval = std::chrono::milliseconds(aMilliseconds);
			m_expiresAt = std::chrono::steady_clock::now() + m_interval;
		}

		bool
		HasExpired()
		{
			std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();
			if(currentTime < m_expiresAt)
				return false;

			m_expiresAt = currentTime + m_interval;

			return true;
		}

	private:

		std::chrono::milliseconds							m_interval;
		std::chrono::time_point<std::chrono::steady_clock>	m_expiresAt;
	};
	
}