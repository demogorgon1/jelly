#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/ScopedTimeSampler.h>

#include "Stats.h"

namespace jelly
{

	Stats::Stats(
		const ExtraApplicationStats& aExtraApplicationStats)
		: m_threadCount(0)
		, m_updateCount(0)
		, m_extraApplicationStats(aExtraApplicationStats)
	{
		for(uint32_t i = 0; i < (uint32_t)Stat::NUM_TYPES; i++)
			m_typeCount[i] = 0;

		m_totalStatCount = (size_t)aExtraApplicationStats.m_infoCount + (size_t)Stat::NUM_IDS;
		m_typeIndices.resize(m_totalStatCount);

		for(uint32_t i = 0; i < (uint32_t)m_totalStatCount; i++)
		{
			const Stat::Info* info = _GetStatInfo(i);

			JELLY_ASSERT((uint32_t)info->m_type < (uint32_t)Stat::NUM_TYPES);
			size_t typeIndex = m_typeCount[info->m_type];
			m_typeIndices[i] = typeIndex;
			
			switch(info->m_type)
			{
			case Stat::TYPE_COUNTER:
				if(info->m_cRateMA > 0)
				{
					m_counterMovingAverages.push_back(std::make_unique<CounterMovingAverage>(
						(size_t)info->m_cRateMA,
						typeIndex));
				}
				break;

			default:
				break;
			}

			m_typeCount[info->m_type]++;
		}

		m_counters.resize(m_typeCount[Stat::TYPE_COUNTER]);
		m_samplers.resize(m_typeCount[Stat::TYPE_SAMPLER]);
		m_gauges.resize(m_typeCount[Stat::TYPE_GAUGE]);

		_InitData(m_collectedData);
	}

	Stats::~Stats()
	{

	}

	//----------------------------------------------------------------------------------

	void		
	Stats::Emit_UInt64(
		uint32_t							aId,
		uint64_t							aValue,
		const std::optional<Stat::Type>&	aExpectedType) 
	{
		if(aId != UINT32_MAX)
		{
			const Stat::Info* info = _GetStatInfo(aId);
			JELLY_ASSERT(!aExpectedType.has_value() || aExpectedType == info->m_type);
			size_t typeIndex = m_typeIndices[aId];

			_GetCurrentThread()->Emit(aValue, info, typeIndex);
		}
	}

	void				
	Stats::Update() 
	{
		ScopedTimeSampler timer(this, Stat::ID_STATS_UPDATE_TIME);

		std::chrono::time_point<std::chrono::steady_clock> currentTime = std::chrono::steady_clock::now();

		_CollectDataFromThreads();

		_UpdateCounters(currentTime);
		_UpdateSamplers(currentTime);
		_UpdateGauges(currentTime);

		m_lastUpdateTime = currentTime;

		m_updateCount++;
	}
	
	Stats::Counter
	Stats::GetCounter(
		uint32_t		aId) const 
	{
		const Stat::Info* info = _GetStatInfo(aId);
		JELLY_ASSERT(info->m_type == Stat::TYPE_COUNTER);
		size_t typeIndex = m_typeIndices[aId];
		JELLY_ASSERT(typeIndex < (size_t)m_counters.size());
		return m_counters[typeIndex];
	}
	
	Stats::Sampler
	Stats::GetSampler(
		uint32_t		aId) const
	{
		const Stat::Info* info = _GetStatInfo(aId);
		JELLY_ASSERT(info->m_type == Stat::TYPE_SAMPLER);
		size_t typeIndex = m_typeIndices[aId];
		JELLY_ASSERT(typeIndex < (size_t)m_samplers.size());
		return m_samplers[typeIndex];
	}

	Stats::Gauge
	Stats::GetGauge(
		uint32_t		aId) const
	{
		const Stat::Info* info = _GetStatInfo(aId);
		JELLY_ASSERT(info->m_type == Stat::TYPE_GAUGE);
		size_t typeIndex = m_typeIndices[aId];
		JELLY_ASSERT(typeIndex < (size_t)m_gauges.size());
		return m_gauges[typeIndex];
	}

	uint32_t			
	Stats::GetIdByString(
		const char*		aString) const
	{
		for(uint32_t i = 0; i < m_totalStatCount; i++)
		{
			const Stat::Info* info = _GetStatInfo(i);

			if(strcmp(info->m_id, aString) == 0)
				return i;
		}

		return UINT32_MAX;
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
	Stats::Thread::Emit(
		uint64_t			aValue,
		const Stat::Info*	aInfo,
		size_t				aTypeIndex)
	{
		switch(aInfo->m_type)
		{
		case Stat::TYPE_COUNTER:
			{
				std::lock_guard lock(m_lock);

				JELLY_ASSERT(aTypeIndex < m_writeData->m_counterData.size());
				CounterData* counter = &m_writeData->m_counterData[aTypeIndex];

				counter->m_value += aValue;
				counter->m_count++;
			}
			break;

		case Stat::TYPE_SAMPLER:
			{
				std::lock_guard lock(m_lock);

				JELLY_ASSERT(aTypeIndex < m_writeData->m_samplerData.size());
				SamplerData* sampler = &m_writeData->m_samplerData[aTypeIndex];

				if(sampler->m_count > 0)
				{
					sampler->m_min = std::min<uint64_t>(sampler->m_min, aValue);
					sampler->m_max = std::max<uint64_t>(sampler->m_max, aValue);
				}
				else
				{
					sampler->m_min = aValue;
					sampler->m_max = aValue;
				}

				sampler->m_sum += aValue;
				sampler->m_count++;
			}
			break;

		case Stat::TYPE_GAUGE:
			{
				std::lock_guard lock(m_lock);

				JELLY_ASSERT(aTypeIndex < m_writeData->m_gaugeData.size());
				GaugeData* gauge = &m_writeData->m_gaugeData[aTypeIndex];

				gauge->m_value = aValue;
				gauge->m_isSet = true;
			}
			break;

		default:
			JELLY_ASSERT(false);
		}
	}

	Stats::Data*
	Stats::Thread::SwapAndGetReadData()
	{
		if(m_readData)
			m_readData->ResetThread();

		{
			std::lock_guard lock(m_lock);

			std::swap(m_readData, m_writeData);
		}

		return m_readData.get();
	}

	//----------------------------------------------------------------------------------

	void		
	Stats::_InitData(
		Data&				aData)
	{
		aData.Init(
			m_typeCount[Stat::TYPE_COUNTER],
			m_typeCount[Stat::TYPE_SAMPLER],
			m_typeCount[Stat::TYPE_GAUGE]);
	}

	Stats::Thread* 
	Stats::_GetCurrentThread()
	{
		uint32_t currentThreadIndex = ThreadIndex::Get();
		JELLY_ASSERT(currentThreadIndex < ThreadIndex::MAX_THREADS);
		Thread* currentThread = &m_threads[currentThreadIndex];

		if(!currentThread->m_initialized)
		{
			currentThread->m_readData = std::make_unique<Data>();
			_InitData(*currentThread->m_readData);

			currentThread->m_writeData = std::make_unique<Data>();
			_InitData(*currentThread->m_writeData);

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

	void		
	Stats::_CollectDataFromThreads()
	{
		uint32_t threadCount = 0;

		{
			std::lock_guard lock(m_threadCountLock);

			threadCount = m_threadCount;
		}

		m_collectedData.ResetCollected();

		for (uint32_t i = 0; i < threadCount; i++)
		{
			Data* threadReadData = m_threads[i].SwapAndGetReadData();

			if (threadReadData != NULL)
				m_collectedData.Add(*threadReadData);
		}
	}

	void		
	Stats::_UpdateCounters(	
		std::chrono::time_point<std::chrono::steady_clock>	aCurrentTime)
	{
		JELLY_ASSERT(m_collectedData.m_counterData.size() == m_counters.size());

		if (m_updateCount > 0)
		{
			uint32_t timeSinceLastUpdate = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(aCurrentTime - m_lastUpdateTime).count();

			if (timeSinceLastUpdate > 0)
			{
				for (size_t i = 0; i < m_counters.size(); i++) 
				{
					uint64_t rate = (m_collectedData.m_counterData[i].m_value * 1000) / timeSinceLastUpdate;

					m_counters[i].m_rate = rate;
					m_counters[i].m_rateMA = 0;
				}
			}
		}

		for(std::unique_ptr<CounterMovingAverage>& t : m_counterMovingAverages)
			m_counters[t->m_index].m_rateMA = t->m_movingAverage.Update(m_counters[t->m_index].m_rate);

		for (size_t i = 0; i < m_counters.size(); i++)
			m_counters[i].m_value += m_collectedData.m_counterData[i].m_value;
	}
	
	void		
	Stats::_UpdateSamplers(
		std::chrono::time_point<std::chrono::steady_clock>	/*aCurrentTime*/)
	{
		JELLY_ASSERT(m_collectedData.m_samplerData.size() == m_samplers.size());

		for (size_t i = 0; i < m_samplers.size(); i++)
		{
			Sampler& sampler = m_samplers[i];
			const SamplerData& collected = m_collectedData.m_samplerData[i];

			sampler.m_count = collected.m_count;

			if (sampler.m_count > 0)
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
	}
	
	void		
	Stats::_UpdateGauges(
		std::chrono::time_point<std::chrono::steady_clock>	/*aCurrentTime*/)
	{
		JELLY_ASSERT(m_collectedData.m_gaugeData.size() == m_gauges.size());

		for (size_t i = 0; i < m_gauges.size(); i++)
		{
			Gauge& gauge = m_gauges[i];
			const GaugeData& collected = m_collectedData.m_gaugeData[i];

			if (collected.m_isSet)
				gauge.m_value = collected.m_value;
		}
	}

	const Stat::Info* 
	Stats::_GetStatInfo(
		uint32_t											aId) const
	{
		if(aId < (uint32_t)Stat::NUM_IDS)
			return Stat::GetInfo(aId);

		uint32_t extraApplicationStatIndex = aId - (uint32_t)Stat::NUM_IDS;
		JELLY_ASSERT(extraApplicationStatIndex < m_extraApplicationStats.m_infoCount);
		return &m_extraApplicationStats.m_info[extraApplicationStatIndex];
	}


}