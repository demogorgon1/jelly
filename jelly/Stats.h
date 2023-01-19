#pragma once

#include <jelly/IStats.h>
#include <jelly/Stat.h>

#include "MovingAverage.h"
#include "ThreadIndex.h"

namespace jelly
{

	class Stats
		: public IStats
	{
	public:
							Stats();
		virtual				~Stats();

		// Virtual interface
		void				AddCounter_UInt64(
								uint32_t		aId,
								uint64_t		aValue) override;
		void				SampleTime_UInt64(
								uint32_t		aId,
								uint64_t		aTime) override;
		void				Update() override;
		Counter				GetCounter(
								uint32_t		aId) override;
		TimeSampler			GetTimeSampler(
								uint32_t		aId) override;

	private:

		struct CounterData
		{
			CounterData()
				: m_value(0)
				, m_count(0)
			{

			}

			void
			Add(
				const CounterData&				aOther)
			{
				m_value += aOther.m_value;
				m_count += aOther.m_count;
			}

			// Public data
			size_t											m_value;
			size_t											m_count;
		};

		struct TimeSamplerData
		{
			TimeSamplerData()
				: m_sum(0)
				, m_count(0)
				, m_min(0)
				, m_max(0)
			{

			}

			void
			Add(
				const TimeSamplerData&			aOther)
			{
				if(aOther.m_count > 0)
				{
					if(m_count > 0)
					{
						m_min = std::min<size_t>(m_min, aOther.m_min);
						m_max = std::max<size_t>(m_max, aOther.m_max);
					}
					else
					{
						m_min = aOther.m_min;
						m_max = aOther.m_max;
					}

					m_sum += aOther.m_sum;
					m_count += aOther.m_count;
				}
			}

			// Public data
			size_t											m_sum;
			size_t											m_count;
			size_t											m_min;
			size_t											m_max;
		};

		struct Data
		{
			Data()
			{
			}

			void
			Reset()
			{
				memset(m_counterData, 0, sizeof(m_counterData));
				memset(m_timeSamplerData, 0, sizeof(m_timeSamplerData));
			}

			void
			Add(
				const Data&						aOther)
			{
				for(uint32_t i = 0; i < (uint32_t)Stat::NUM_COUNTERS; i++)
					m_counterData[i].Add(aOther.m_counterData[i]);

				for(uint32_t i = 0; i < (uint32_t)Stat::NUM_TIME_SAMPLERS; i++)
					m_timeSamplerData[i].Add(aOther.m_timeSamplerData[i]);
			}

			// Public data
			CounterData										m_counterData[Stat::NUM_COUNTERS];
			TimeSamplerData									m_timeSamplerData[Stat::NUM_TIME_SAMPLERS];
		};

		struct Thread
		{
						Thread();
						~Thread();

			void		AddCounter(
							uint32_t			aId,
							uint64_t			aValue);
			void		SampleTime(
							uint32_t			aId,
							uint64_t			aTime);
			Data*		SwapAndGetReadData();

			// Public data
			bool													m_initialized;

			std::mutex												m_lock;
			std::unique_ptr<Data>									m_readData;
			std::unique_ptr<Data>									m_writeData;
		};

		Thread														m_threads[ThreadIndex::MAX_THREADS];

		std::mutex													m_threadCountLock;
		uint32_t													m_threadCount;

		Counter														m_counters[Stat::NUM_COUNTERS];
		TimeSampler													m_timeSamplers[Stat::NUM_TIME_SAMPLERS];

		std::unique_ptr<MovingAverage<uint64_t>>					m_counterMovingAverages[Stat::NUM_COUNTERS];

		std::chrono::time_point<std::chrono::steady_clock>			m_lastUpdateTime;
		size_t														m_updateCount;

		Thread*		_GetCurrentThread();
	};

}