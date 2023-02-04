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
				Compression::Id					aCompressionId)
				: DefaultHost(aRoot, "test", aCompressionId)
				, m_timeStamp(0)
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