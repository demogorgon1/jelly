#pragma once

#include "ErrorUtils.h"

namespace jelly
{

	namespace Stat
	{

		enum Id : uint32_t
		{
			ID_DISK_WRITE_WAL_BYTES,
			ID_DISK_WRITE_STORE_BYTES,
			ID_FLUSH_PENDING_WAL_TIME,
			ID_WAL_COUNT,

			NUM_IDS
		};

		enum Type 
		{
			TYPE_COUNTER,
			TYPE_SAMPLER,
			TYPE_GAUGE,

			NUM_TYPES
		};

		struct Info
		{
			Type			m_type;
			const char*		m_id;
			uint32_t		m_counterRateMovingAverage;
		};

		// IMPORTANT: Order must match the Counter enum
		static constexpr Info INFO[] =
		{
			//                                      | m_type       | m_id                    | m_counterRateMovingAverage
			//--------------------------------------+--------------+-------------------------+-----------------------------
			/* ID_DISK_WRITE_WAL_BYTES */		    { TYPE_COUNTER, "disk_write_wal_bytes",	   10 },
			/* ID_DISK_WRITE_STORE_BYTES */	        { TYPE_COUNTER, "disk_write_store_bytes",  10 },
			/* ID_FLUSH_PENDING_WAL_TIME */	        { TYPE_SAMPLER, "flush_pending_wal_time",  0 },
			/* ID_WAL_COUNT */                      { TYPE_GAUGE,   "wal_count",               0 }
		};

		static_assert(sizeof(INFO) == sizeof(Info) * (size_t)NUM_IDS);

		inline const Info*
		GetInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_IDS);
			return &INFO[aId];
		}

	}

}