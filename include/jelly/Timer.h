#pragma once

#include "ConfigProxy.h"

namespace jelly
{

	// Egg clock timer
	class Timer
	{
	public:
		Timer(
			uint32_t		aMilliseconds = 0)
			: m_config(NULL)
			, m_configId(0)
		{
			SetTimeout(aMilliseconds);
		}

		Timer(
			ConfigProxy*	aConfig,
			uint32_t		aConfigId)
			: m_config(aConfig)
			, m_configId(aConfigId)
		{
			m_interval = std::chrono::milliseconds(m_config->GetInterval((Config::Id)aConfigId));
			m_expiresAt = std::chrono::steady_clock::now() + m_interval;
		}

		~Timer()
		{

		}

		void
		SetTimeout(
			uint32_t		aMilliseconds) noexcept
		{
			JELLY_ASSERT(m_config == NULL);
			m_interval = std::chrono::milliseconds(aMilliseconds);
			m_expiresAt = std::chrono::steady_clock::now() + m_interval;
		}

		bool
		HasExpired() noexcept
		{
			std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();
			if(currentTime < m_expiresAt)
				return false;

			if(m_config != NULL)
				m_interval = std::chrono::milliseconds(m_config->GetInterval((Config::Id)m_configId));

			m_expiresAt = currentTime + m_interval;

			return true;
		}

		void
		Reset() noexcept
		{
			if (m_config != NULL)
				m_interval = std::chrono::milliseconds(m_config->GetInterval((Config::Id)m_configId));

			m_expiresAt = std::chrono::steady_clock::now() + m_interval;
		}

	private:

		std::chrono::milliseconds							m_interval;
		std::chrono::time_point<std::chrono::steady_clock>	m_expiresAt;

		uint32_t											m_configId;
		ConfigProxy*										m_config;
	};
	
}