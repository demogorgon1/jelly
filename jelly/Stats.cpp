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
		memset(m_timeSamplers, 0, sizeof(m_timeSamplers));

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
	Stats::SampleTime_UInt64(
		uint32_t		aId,
		uint64_t		aTime) 
	{
		_GetCurrentThread()->SampleTime(aId, aTime);
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

		for (uint32_t i = 0; i < (uint32_t)Stat::NUM_TIME_SAMPLERS; i++)
		{
			TimeSampler& timeSampler = m_timeSamplers[i];
			const TimeSamplerData& collected = collectedData.m_timeSamplerData[i];

			timeSampler.m_count = collected.m_count;
			
			if(timeSampler.m_count > 0)
			{
				timeSampler.m_avg = collected.m_sum / collected.m_count;
				timeSampler.m_max = collected.m_max;
				timeSampler.m_min = collected.m_min;
			}
			else
			{
				timeSampler.m_avg = 0;
				timeSampler.m_max = 0;
				timeSampler.m_min = 0;
			}
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
	
	Stats::TimeSampler
	Stats::GetTimeSampler(
		uint32_t		aId)
	{
		JELLY_ASSERT(aId < (uint32_t)Stat::NUM_TIME_SAMPLERS);
		return m_timeSamplers[aId];
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
	Stats::Thread::SampleTime(
		uint32_t		aId,
		uint64_t		aTime)
	{
		std::lock_guard lock(m_lock);

		JELLY_ASSERT(aId < Stat::NUM_TIME_SAMPLERS);
		TimeSamplerData* timeSampler = &m_writeData->m_timeSamplerData[aId];

		if(timeSampler->m_count > 0)
		{
			timeSampler->m_min = std::min<uint64_t>(timeSampler->m_min, aTime);
			timeSampler->m_max = std::max<uint64_t>(timeSampler->m_max, aTime);
		}
		else
		{
			timeSampler->m_min = aTime;
			timeSampler->m_max = aTime;
		}

		timeSampler->m_sum += aTime;
		timeSampler->m_count++;
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