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

		enum Sampler : uint32_t
		{
			SAMPLER_FLUSH_PENDING_WAL_TIME,

			NUM_SAMPLERS
		};

		enum Gauge : uint32_t
		{
			GAUGE_WAL_COUNT,

			NUM_GAUGES
		};

		struct CounterInfo
		{
			const char*		m_id;
			uint32_t		m_rateMovingAverage;
		};

		struct SamplerInfo
		{
			const char*		m_id;
		};

		struct GaugeInfo
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

		// IMPORTANT: Order must match the Sampler enum
		static constexpr SamplerInfo SAMPLER_INFO[] =
		{
			//                                      | Id                     
			//--------------------------------------+-------------------------------------
			/* SAMPLER_FLUSH_PENDING_WAL_TIME */    { "flush_pending_wal_time" },
		};

		// IMPORTANT: Order must match the Gauge enum
		static constexpr GaugeInfo GAUGE_INFO[] =
		{
			//                                      | Id                     
			//--------------------------------------+-------------------------------------
			/* GAUGE_WAL_COUNT */                   { "wal_count" },
		};

		static_assert(sizeof(COUNTER_INFO) == sizeof(CounterInfo) * (size_t)NUM_COUNTERS);
		static_assert(sizeof(SAMPLER_INFO) == sizeof(SamplerInfo) * (size_t)NUM_SAMPLERS);
		static_assert(sizeof(GAUGE_INFO) == sizeof(GaugeInfo) * (size_t)NUM_GAUGES);

		inline const CounterInfo*
		GetCounterInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_COUNTERS);
			return &COUNTER_INFO[aId];
		}

		inline const SamplerInfo*
		GetSamplerInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_SAMPLERS);
			return &SAMPLER_INFO[aId];
		}

		inline const GaugeInfo*
		GetGaugeInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_GAUGES);
			return &GAUGE_INFO[aId];
		}

	}

}