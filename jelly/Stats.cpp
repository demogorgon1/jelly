#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>

#include "Stats.h"

namespace jelly
{

	Stats::Stats()
		: m_threadCount(0)
		, m_updateCount(0)
	{
		memset(m_counters, 0, sizeof(m_counters));
		memset(m_samplers, 0, sizeof(m_samplers));
		memset(m_gauges, 0, sizeof(m_gauges));

		// Initialize moving average filters for counters that need them
		for(uint32_t i = 0; i < (uint32_t)Stat::NUM_COUNTERS; i++)
		{
			const Stat::CounterInfo* counterInfo = Stat::GetCounterInfo(i);

			if(counterInfo->m_rateMovingAverage > 0)
				m_counterMovingAverages[i] = std::make_unique<MovingAverage<uint64_t>>((size_t)counterInfo->m_rateMovingAverage);
		}
	}

	Stats::~Stats()
	{

	}

	//----------------------------------------------------------------------------------

	void		
	Stats::AddCounter_UInt64(
		uint32_t		aId,
		uint64_t		aValue) 
	{
		_GetCurrentThread()->AddCounter(aId, aValue);
	}

	void				
	Stats::Sample_UInt64(
		uint32_t		aId,
		uint64_t		aValue) 
	{
		_GetCurrentThread()->Sample(aId, aValue);
	}

	void				
	Stats::SetGauge_UInt64(
		uint32_t		aId,
		uint64_t		aValue) 
	{
		_GetCurrentThread()->SetGauge(aId, aValue);
	}

	void				
	Stats::Update() 
	{
		uint32_t threadCount = 0;
	
		{
			std::lock_guard lock(m_threadCountLock);

			threadCount = m_threadCount;
		}

		Data collectedData;

		for(uint32_t i = 0; i < threadCount; i++)
		{
			Data* threadReadData = m_threads[i].SwapAndGetReadData();

			if(threadReadData != NULL)
				collectedData.Add(*threadReadData);
		}

		std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();

		if(m_updateCount > 0)
		{
			uint32_t timeSinceLastUpdate = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastUpdateTime).count();

			if(timeSinceLastUpdate > 0)
			{
				for(uint32_t i = 0; i < (uint32_t)Stat::NUM_COUNTERS; i++)
				{
					uint64_t rate = (collectedData.m_counterData[i].m_value * 1000) / timeSinceLastUpdate;
					
					m_counters[i].m_rate = rate;

					if(m_counterMovingAverages[i])
						m_counters[i].m_rateMA = m_counterMovingAverages[i]->Update(rate);
					else
						m_counters[i].m_rateMA = rate;
				}
			}
		}

		for (uint32_t i = 0; i < (uint32_t)Stat::NUM_COUNTERS; i++)
			m_counters[i].m_value += collectedData.m_counterData[i].m_value;

		for (uint32_t i = 0; i < (uint32_t)Stat::NUM_SAMPLERS; i++)
		{
			Sampler& sampler = m_samplers[i];
			const SamplerData& collected = collectedData.m_samplerData[i];

			sampler.m_count = collected.m_count;
			
			if(sampler.m_count > 0)
			{
				sampler.m_avg = collected.m_sum / collected.m_count;
				sampler.m_max = collected.m_max;
				sampler.m_min = collected.m_min;
			}
			else
			{
				sampler.m_avg = 0;
				sampler.m_max = 0;
				sampler.m_min = 0;
			}
		}

		for(uint32_t i = 0; i < (uint32_t)Stat::NUM_GAUGES; i++)
		{
			Gauge& gauge = m_gauges[i];
			const GaugeData& collected = collectedData.m_gaugeData[i];

			if(collected.m_isSet)
				gauge.m_value = collected.m_value;
		}

		m_lastUpdateTime = currentTime;
		m_updateCount++;
	}
	
	Stats::Counter
	Stats::GetCounter(
		uint32_t		aId)
	{
		JELLY_ASSERT(aId < (uint32_t)Stat::NUM_COUNTERS);
		return m_counters[aId];
	}
	
	Stats::Sampler
	Stats::GetSampler(
		uint32_t		aId)
	{
		JELLY_ASSERT(aId < (uint32_t)Stat::NUM_SAMPLERS);
		return m_samplers[aId];
	}

	Stats::Gauge
	Stats::GetGauge(
		uint32_t		aId)
	{
		JELLY_ASSERT(aId < (uint32_t)Stat::NUM_GAUGES);
		return m_gauges[aId];
	}

	//----------------------------------------------------------------------------------

	Stats::Thread::Thread()
		: m_initialized(false)
	{

	}

	Stats::Thread::~Thread()
	{

	}

	void	
	Stats::Thread::AddCounter(
		uint32_t		aId,
		uint64_t		aValue)
	{
		std::lock_guard lock(m_lock);

		JELLY_ASSERT(aId < Stat::NUM_COUNTERS);
		CounterData* counter = &m_writeData->m_counterData[aId];

		counter->m_value += aValue;
		counter->m_count++;
	}

	void		
	Stats::Thread::Sample(
		uint32_t		aId,
		uint64_t		aSample)
	{
		std::lock_guard lock(m_lock);

		JELLY_ASSERT(aId < Stat::NUM_SAMPLERS);
		SamplerData* sampler = &m_writeData->m_samplerData[aId];

		if(sampler->m_count > 0)
		{
			sampler->m_min = std::min<uint64_t>(sampler->m_min, aSample);
			sampler->m_max = std::max<uint64_t>(sampler->m_max, aSample);
		}
		else
		{
			sampler->m_min = aSample;
			sampler->m_max = aSample;
		}

		sampler->m_sum += aSample;
		sampler->m_count++;
	}

	void		
	Stats::Thread::SetGauge(
		uint32_t		aId,
		uint64_t		aValue)
	{
		std::lock_guard lock(m_lock);

		JELLY_ASSERT(aId < Stat::NUM_GAUGES);
		GaugeData* gauge = &m_writeData->m_gaugeData[aId];

		gauge->m_value = aValue;
		gauge->m_isSet = true;
	}

	Stats::Data*
	Stats::Thread::SwapAndGetReadData()
	{
		if(m_readData)
			m_readData->Reset();

		{
			std::lock_guard lock(m_lock);

			std::swap(m_readData, m_writeData);
		}

		return m_readData.get();
	}

	//----------------------------------------------------------------------------------

	Stats::Thread* 
	Stats::_GetCurrentThread()
	{
		uint32_t currentThreadIndex = ThreadIndex::Get();
		JELLY_ASSERT(currentThreadIndex < ThreadIndex::MAX_THREADS);
		Thread* currentThread = &m_threads[currentThreadIndex];

		if(!currentThread->m_initialized)
		{
			currentThread->m_readData = std::make_unique<Data>();
			currentThread->m_writeData = std::make_unique<Data>();

			currentThread->m_initialized = true;

			// Updata thread count
			{
				std::lock_guard lock(m_threadCountLock);

				if(currentThreadIndex >= m_threadCount)
					m_threadCount = currentThreadIndex + 1;
			}
		}

		return currentThread;
	}

}