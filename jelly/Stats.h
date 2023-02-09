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
		struct ExtraApplicationStats
		{
			ExtraApplicationStats(
				const Stat::Info*										aInfo = NULL,
				uint32_t												aInfoCount = 0)
				: m_info(aInfo)
				, m_infoCount(aInfoCount)
			{

			}

			const Stat::Info*		m_info;
			uint32_t				m_infoCount;
		};

								Stats(
									const ExtraApplicationStats&		aExtraApplicationStats = ExtraApplicationStats());
		virtual					~Stats();

		//---------------------------------------------------------------------------
		// IStats implementation
		void					Emit_UInt64(
									uint32_t							aId,
									uint64_t							aValue,
									const std::optional<Stat::Type>&	aExpectedType) override;
		void					Update() override;
		Counter					GetCounter(
									uint32_t							aId) const override;
		Sampler					GetSampler(
									uint32_t							aId) const override;
		SamplerHistogramView	GetSamplerHistogramView(
									uint32_t							aId) const override;
		Gauge					GetGauge(
									uint32_t							aId) const override;
		uint32_t				GetIdByString(
									const char*							aString) const override;
		const Stat::Info*		GetInfo(
									uint32_t							aId) const override;

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
			Init(
				size_t							aNumCounters,
				size_t							aNumSamplers,
				size_t							aNumGauges,
				size_t							aNumSamplerHistogramBuckets)
			{
				JELLY_ASSERT(m_counterData.size() == 0);
				JELLY_ASSERT(m_samplerData.size() == 0);
				JELLY_ASSERT(m_gaugeData.size() == 0);
				JELLY_ASSERT(m_samplerHistogramData.size() == 0);

				m_counterData.resize(aNumCounters);
				m_samplerData.resize(aNumSamplers);
				m_gaugeData.resize(aNumGauges);
				m_samplerHistogramData.resize(aNumSamplerHistogramBuckets);
			}

			void
			ResetThread()
			{
				// Note that we don't reset gauges, they retain their values
				std::fill(m_counterData.begin(), m_counterData.end(), CounterData());
				std::fill(m_samplerData.begin(), m_samplerData.end(), SamplerData());
				std::fill(m_samplerHistogramData.begin(), m_samplerHistogramData.end(), 0);
			}

			void
			ResetCollected()
			{
				std::fill(m_counterData.begin(), m_counterData.end(), CounterData());
				std::fill(m_samplerData.begin(), m_samplerData.end(), SamplerData());
				std::fill(m_gaugeData.begin(), m_gaugeData.end(), GaugeData());
				std::fill(m_samplerHistogramData.begin(), m_samplerHistogramData.end(), 0);
			}

			void
			Add(
				const Data&						aOther)
			{
				JELLY_ASSERT(m_counterData.size() == aOther.m_counterData.size());
				JELLY_ASSERT(m_samplerData.size() == aOther.m_samplerData.size());
				JELLY_ASSERT(m_gaugeData.size() == aOther.m_gaugeData.size());
				JELLY_ASSERT(m_samplerHistogramData.size() == aOther.m_samplerHistogramData.size());

				for(size_t i = 0; i < m_counterData.size(); i++)
					m_counterData[i].Add(aOther.m_counterData[i]);

				for(size_t i = 0; i < m_samplerData.size(); i++)
					m_samplerData[i].Add(aOther.m_samplerData[i]);

				for (size_t i = 0; i < m_gaugeData.size(); i++)
					m_gaugeData[i].Add(aOther.m_gaugeData[i]);

				for(size_t i = 0; i < m_samplerHistogramData.size(); i++)
					m_samplerHistogramData[i] += aOther.m_samplerHistogramData[i];
			}

			// Public data
			std::vector<CounterData>									m_counterData;
			std::vector<SamplerData>									m_samplerData;
			std::vector<GaugeData>										m_gaugeData;

			std::vector<uint32_t>										m_samplerHistogramData;
		};

		struct CounterMovingAverage
		{
			CounterMovingAverage(
				size_t							aSize,
				size_t							aIndex)
				: m_movingAverage(aSize)
				, m_index(aIndex)				
			{
			
			}

			// Public data
			MovingAverage<uint64_t>									m_movingAverage;
			size_t													m_index;
		};

		struct SamplerHistogram
		{
			SamplerHistogram(
				const std::vector<uint64_t>*	aBuckets,
				size_t							aOffset)
				: m_buckets(aBuckets)
				, m_offset(aOffset)
			{

			}

			size_t
			GetBucketIndex(
				uint64_t						aValue) const
			{
				size_t bucketIndex = m_offset;
				for(size_t i = 0; i < m_buckets->size() - 1 && aValue > m_buckets->at(i); i++)
					bucketIndex++;

				return bucketIndex;
			}

			bool
			IsSet() const
			{
				return m_buckets != NULL;
			}
			
			// Public data
			const std::vector<uint64_t>*							m_buckets;
			size_t													m_offset;
		};

		struct Thread
		{
						Thread();
						~Thread();

			void		Emit(
							uint64_t			aValue,
							const Stat::Info*	aInfo,
							size_t				aTypeIndex);
			Data*		SwapAndGetReadData();

			// Public data
			bool													m_initialized;
			const std::vector<SamplerHistogram>*					m_samplerHistograms;

			std::mutex												m_lock;
			std::unique_ptr<Data>									m_readData;
			std::unique_ptr<Data>									m_writeData;
		};

		ExtraApplicationStats										m_extraApplicationStats;

		Thread														m_threads[ThreadIndex::MAX_THREADS];

		std::mutex													m_threadCountLock;
		uint32_t													m_threadCount;

		std::vector<Counter>										m_counters;
		std::vector<Sampler>										m_samplers;
		std::vector<Gauge>											m_gauges;

		size_t														m_totalStatCount;
		size_t														m_totalSamplerHistogramBucketCount;

		std::vector<size_t>											m_typeIndices;
		size_t														m_typeCount[Stat::NUM_TYPES];
		
		std::vector<std::unique_ptr<CounterMovingAverage>>			m_counterMovingAverages;
		std::vector<SamplerHistogram>								m_samplerHistograms;

		std::chrono::time_point<std::chrono::steady_clock>			m_lastUpdateTime;
		size_t														m_updateCount;

		Data														m_collectedData;

		void				_InitData(
								Data&												aData);
		Thread*				_GetCurrentThread();
		void				_CollectDataFromThreads();
		void				_UpdateCounters(
								std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime);
		void				_UpdateSamplers(
								std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime);
		void				_UpdateGauges(
								std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime);
		const Stat::Info*	_GetStatInfo(
								uint32_t											aId) const;
	};

}