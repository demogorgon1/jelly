#pragma once

#include "ErrorUtils.h"

namespace jelly
{

	namespace Stat
	{

		enum Counter : uint32_t
		{
			COUNTER_DISK_WRITE_WAL_BYTES,
			COUNTER_DISK_WRITE_STORE_BYTES,

			NUM_COUNTERS
		};

		enum TimeSampler : uint32_t
		{
			TIME_SAMPLER_FLUSH_PENDING_WAL,

			NUM_TIME_SAMPLERS
		};

		struct CounterInfo
		{
			const char*		m_id;
			uint32_t		m_rateMovingAverage;
		};

		struct TimeSamplerInfo
		{
			const char*		m_id;
		};

		// IMPORTANT: Order must match the Counter enum
		static constexpr CounterInfo COUNTER_INFO[] =
		{
			//                                      | Id                       | Rate moving average (updates)
			//--------------------------------------+--------------------------+-------------------------------
			/* COUNTER_DISK_WRITE_WAL_BYTES */		{ "disk_write_wal_bytes",	 10 },
			/* COUNTER_DISK_WRITE_STORE_BYTES */	{ "disk_write_store_bytes",	 10 }
		};

		// IMPORTANT: Order must match the TimeSampler enum
		static constexpr TimeSamplerInfo TIME_SAMPLER_INFO[] =
		{
			//                                     | Id                     
			//-------------------------------------+-------------------------------------
			/* TIME_SAMPLER_FLUSH_PENDING_WALS */	{ "flush_pending_wal" },
		};

		static_assert(sizeof(COUNTER_INFO) == sizeof(CounterInfo) * (size_t)NUM_COUNTERS);
		static_assert(sizeof(TIME_SAMPLER_INFO) == sizeof(TimeSamplerInfo) * (size_t)NUM_TIME_SAMPLERS);

		inline const CounterInfo*
		GetCounterInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_COUNTERS);
			return &COUNTER_INFO[aId];
		}

		inline const TimeSamplerInfo*
		GetTimeSamplerInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_TIME_SAMPLERS);
			return &TIME_SAMPLER_INFO[aId];
		}
	}

}