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
		void				Sample_UInt64(
								uint32_t		aId,
								uint64_t		aValue) override;
		void				SetGauge_UInt64(
								uint32_t		aId,
								uint64_t		aValue) override;
		void				Update() override;
		Counter				GetCounter(
								uint32_t		aId) override;
		Sampler				GetSampler(
								uint32_t		aId) override;
		Gauge				GetGauge(
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
			uint64_t										m_value;
			uint64_t										m_count;
		};

		struct SamplerData
		{
			SamplerData()
				: m_sum(0)
				, m_count(0)
				, m_min(0)
				, m_max(0)
			{

			}

			void
			Add(
				const SamplerData&				aOther)
			{
				if(aOther.m_count > 0)
				{
					if(m_count > 0)
					{
						m_min = std::min<uint64_t>(m_min, aOther.m_min);
						m_max = std::max<uint64_t>(m_max, aOther.m_max);
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
			uint64_t										m_sum;
			uint64_t										m_count;
			uint64_t										m_min;
			uint64_t										m_max;
		};

		struct GaugeData
		{
			GaugeData()
				: m_value(0)
				, m_isSet(false)
			{

			}

			void
			Add(
				const GaugeData&				aOther)
			{
				m_value += aOther.m_value;

				m_isSet = m_isSet || aOther.m_isSet;
			}

			// Public data
			uint64_t										m_value;
			bool											m_isSet;
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
				memset(m_samplerData, 0, sizeof(m_samplerData));
				memset(m_gaugeData, 0, sizeof(m_gaugeData));
			}

			void
			Add(
				const Data&						aOther)
			{
				for(uint32_t i = 0; i < (uint32_t)Stat::NUM_COUNTERS; i++)
					m_counterData[i].Add(aOther.m_counterData[i]);

				for(uint32_t i = 0; i < (uint32_t)Stat::NUM_SAMPLERS; i++)
					m_samplerData[i].Add(aOther.m_samplerData[i]);

				for (uint32_t i = 0; i < (uint32_t)Stat::NUM_GAUGES; i++)
					m_gaugeData[i].Add(aOther.m_gaugeData[i]);
			}

			// Public data
			CounterData										m_counterData[Stat::NUM_COUNTERS];
			SamplerData										m_samplerData[Stat::NUM_SAMPLERS];
			GaugeData										m_gaugeData[Stat::NUM_GAUGES];
		};

		struct Thread
		{
						Thread();
						~Thread();

			void		AddCounter(
							uint32_t			aId,
							uint64_t			aValue);
			void		Sample(
							uint32_t			aId,
							uint64_t			aValue);
			void		SetGauge(
							uint32_t			aId,
							uint64_t			aValue);
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
		Sampler														m_samplers[Stat::NUM_SAMPLERS];
		Gauge														m_gauges[Stat::NUM_GAUGES];

		std::unique_ptr<MovingAverage<uint64_t>>					m_counterMovingAverages[Stat::NUM_COUNTERS];

		std::chrono::time_point<std::chrono::steady_clock>			m_lastUpdateTime;
		size_t														m_updateCount;

		Thread*		_GetCurrentThread();
	};

}