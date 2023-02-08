#pragma once

/**
 * \file Stat.h
 * 
 * Database statistics definitions.
 */

#include "ErrorUtils.h"

namespace jelly
{

	// Database statistics definitions
	namespace Stat
	{

		//! Unique statistics identifiers. Look at the source code for details.
		enum Id : uint32_t
		{
			ID_DISK_WRITE_LOCK_WAL_BYTES,
			ID_DISK_WRITE_LOCK_STORE_BYTES,
			ID_DISK_WRITE_BLOB_WAL_BYTES,
			ID_DISK_WRITE_BLOB_STORE_BYTES,
			ID_DISK_READ_LOCK_WAL_BYTES,
			ID_DISK_READ_LOCK_STORE_BYTES,
			ID_DISK_READ_BLOB_WAL_BYTES,
			ID_DISK_READ_BLOB_STORE_BYTES,
			ID_FLUSH_PENDING_LOCK_WAL_TIME,
			ID_FLUSH_PENDING_BLOB_WAL_TIME,
			ID_PROCESS_LOCK_REQUESTS_TIME,
			ID_PROCESS_BLOB_REQUESTS_TIME,
			ID_FLUSH_PENDING_LOCK_STORE_TIME,
			ID_FLUSH_PENDING_BLOB_STORE_TIME,
			ID_CLEANUP_LOCK_WALS_TIME,
			ID_CLEANUP_BLOB_WALS_TIME,
			ID_PERFORM_LOCK_COMPACTION_TIME,
			ID_PERFORM_BLOB_COMPACTION_TIME,
			ID_PERFORM_MAJOR_LOCK_COMPACTION_TIME,
			ID_PERFORM_MAJOR_BLOB_COMPACTION_TIME,
			ID_APPLY_LOCK_COMPACTION_TIME,
			ID_APPLY_BLOB_COMPACTION_TIME,
			ID_LOCK_WAL_COUNT,
			ID_BLOB_WAL_COUNT,
			ID_STATS_UPDATE_TIME,
			ID_OBEY_RESIDENT_BLOB_LIMITS_TIME,
			ID_TOTAL_RESIDENT_BLOB_SIZE,
			ID_TOTAL_RESIDENT_BLOB_COUNT,
			ID_TOTAL_HOST_WAL_SIZE,
			ID_TOTAL_HOST_STORE_SIZE,
			ID_AVAILABLE_DISK_SPACE,
			ID_BLOB_SET_TIME,
			ID_BLOB_GET_TIME,
			ID_BLOB_DELETE_TIME,
			ID_LOCK_TIME,
			ID_UNLOCK_TIME,
			ID_LOCK_DELETE_TIME,

			NUM_IDS
		};

		//! Statistics types
		enum Type 
		{
			TYPE_COUNTER,								//!< Incrementing counter (see IStats::Counter)
			TYPE_SAMPLER,								//!< Value sampler (see IStats::Sampler)
			TYPE_GAUGE,									//!< Single value gauge (see IStats::Gauge)

			NUM_TYPES
		};

		//! Information about a statistic
		struct Info
		{
			Type					m_type;
			const char*				m_id;
			uint32_t				m_cRateMA;			//!< Counter: rate moving average window size
			std::vector<uint64_t>	m_sHistBuckets;		//!< Sampler: histogram bucket list
		};

		static const std::vector<uint64_t> TIME_SAMPLER_HISTOGRAM_BUCKETS = 
		{ 
			10, 20,	30,	40,	50,	60,	70,	80, 90,
			100, 200, 300, 400, 500, 600, 700, 800, 900,
			1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
			10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000,
			100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000,
			1000000, 2000000, 3000000, 4000000, 5000000, 6000000, 7000000, 8000000, 9000000,
			10000000
		};

		// IMPORTANT: Order must match the Id enum
		static const Info INFO[] =
		{
			//                                          | m_type       | m_id                               | m_cRateMA | m_sHistBuckets
			//------------------------------------------+--------------+------------------------------------+-----------+----------------------------------
			/* ID_DISK_WRITE_LOCK_WAL_BYTES */		    { TYPE_COUNTER, "disk_write_lock_wal_bytes",          10,         {} },
			/* ID_DISK_WRITE_LOCK_STORE_BYTES */	    { TYPE_COUNTER, "disk_write_lock_store_bytes",        10,         {} },
			/* ID_DISK_WRITE_BLOB_WAL_BYTES */		    { TYPE_COUNTER, "disk_write_blob_wal_bytes",          10,         {} },
			/* ID_DISK_WRITE_BLOB_STORE_BYTES */	    { TYPE_COUNTER, "disk_write_blob_store_bytes",        10,         {} },
			/* ID_DISK_READ_LOCK_WAL_BYTES */		    { TYPE_COUNTER, "disk_read_lock_wal_bytes",           10,         {} },
			/* ID_DISK_READ_LOCK_STORE_BYTES */		    { TYPE_COUNTER, "disk_read_lock_store_bytes",         10,         {} },
			/* ID_DISK_READ_BLOB_WAL_BYTES */		    { TYPE_COUNTER, "disk_read_blob_wal_bytes",           10,         {} },
			/* ID_DISK_READ_BLOB_STORE_BYTES */         { TYPE_COUNTER, "disk_read_blob_store_bytes",         10,         {} },
			/* ID_FLUSH_PENDING_LOCK_WAL_TIME */	    { TYPE_SAMPLER, "flush_pending_lock_wal_time",        0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_FLUSH_PENDING_BLOB_WAL_TIME */	    { TYPE_SAMPLER, "flush_pending_blob_wal_time",        0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PROCESS_LOCK_REQUESTS_TIME */         { TYPE_SAMPLER, "process_lock_requests_time",         0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PROCESS_BLOB_REQUESTS_TIME */         { TYPE_SAMPLER, "process_blob_requests_time",         0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_FLUSH_PENDING_LOCK_STORE_TIME */      { TYPE_SAMPLER, "flush_pending_lock_store_time",      0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_FLUSH_PENDING_BLOB_STORE_TIME */      { TYPE_SAMPLER, "flush_pending_blob_store_time",      0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_CLEANUP_LOCK_WALS_TIME */             { TYPE_SAMPLER, "cleanup_lock_wals_time",             0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_CLEANUP_BLOB_WALS_TIME */             { TYPE_SAMPLER, "cleanup_blob_wals_time",             0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PERFORM_LOCK_COMPACTION_TIME */       { TYPE_SAMPLER, "perform_lock_compaction_time",       0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PERFORM_BLOB_COMPACTION_TIME */       { TYPE_SAMPLER, "perform_blob_compaction_time",       0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PERFORM_MAJOR_LOCK_COMPACTION_TIME */ { TYPE_SAMPLER, "perform_major_lock_compaction_time", 0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_PERFORM_MAJOR_BLOB_COMPACTION_TIME */ { TYPE_SAMPLER, "perform_major_blob_compaction_time", 0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_APPLY_LOCK_COMPACTION_TIME */         { TYPE_SAMPLER, "apply_lock_compaction_time",         0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_APPLY_BLOB_COMPACTION_TIME */         { TYPE_SAMPLER, "apply_blob_compaction_time",         0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LOCK_WAL_COUNT */                     { TYPE_GAUGE,   "lock_wal_count",                     0,          {} },
			/* ID_BLOB_WAL_COUNT */                     { TYPE_GAUGE,   "blob_wal_count",                     0,          {} },
			/* ID_STATS_UPDATE_TIME */				    { TYPE_SAMPLER, "stats_update_time",                  0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_OBEY_RESIDENT_BLOB_LIMITS_TIME */		{ TYPE_SAMPLER, "obey_resident_blob_limits_time",     0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_TOTAL_RESIDENT_BLOB_SIZE */           { TYPE_GAUGE,   "total_resident_blob_size",           0,          {} },
			/* ID_TOTAL_RESIDENT_BLOB_COUNT */          { TYPE_GAUGE,   "total_resident_blob_count",          0,          {} },
			/* ID_TOTAL_HOST_WAL_SIZE */				{ TYPE_GAUGE,   "total_host_wal_size",                0,          {} },
			/* ID_TOTAL_HOST_STORE_SIZE */              { TYPE_GAUGE,   "total_host_store_size",		      0,          {} },
			/* ID_AVAILABLE_DISK_SPACE */               { TYPE_GAUGE,   "available_disk_space",               0,          {} },
			/* ID_BLOB_SET_TIME */						{ TYPE_SAMPLER, "blob_set_time",                      0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BLOB_GET_TIME */						{ TYPE_SAMPLER, "blob_get_time",                      0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_BLOB_DELETE_TIME */					{ TYPE_SAMPLER, "blob_delete_time",                   0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LOCK_TIME */					        { TYPE_SAMPLER, "lock_time",                          0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_UNLOCK_TIME */					    { TYPE_SAMPLER, "unlock_time",                        0,          TIME_SAMPLER_HISTOGRAM_BUCKETS },
			/* ID_LOCK_DELETE_TIME */					{ TYPE_SAMPLER, "lock_delete_time",                   0,          TIME_SAMPLER_HISTOGRAM_BUCKETS }
		};
		
		static_assert(sizeof(INFO) == sizeof(Info) * (size_t)NUM_IDS);

		//! Query information about a statistic
		inline const Info*
		GetInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_IDS);
			return &INFO[aId];
		}

	}

}