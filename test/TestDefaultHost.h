#pragma once

namespace jelly
{
	
	namespace Test
	{

		class TestDefaultHost
			: public DefaultHost
		{
		public:
			TestDefaultHost(
				const char*						aRoot,
				DefaultHost::CompressionMode	aCompressionMode)
				: DefaultHost(aRoot, "test", aCompressionMode)
			{

			}

			~TestDefaultHost()
			{

			}

			// IHost implementation
			uint64_t
			GetTimeStamp() override
			{
				// Predictable current timestamp that always increments by 1 when queried
				return m_timeStamp++;
			}

		private:

			std::atomic_uint64_t	m_timeStamp;
		};

	}

}