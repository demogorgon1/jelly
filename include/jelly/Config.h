#pragma once

/**
 * \file Config.h
 *
 * Database configuration definitions.
 */

#include "ErrorUtils.h"

namespace jelly
{

	namespace Config
	{

		//! Configuration value ids.
		enum Id : uint32_t
		{
			// Node
			ID_WAL_SIZE_LIMIT,
			ID_WAL_CONCURRENCY,

			// BlobNode
			ID_MAX_RESIDENT_BLOB_SIZE,
			ID_MAX_RESIDENT_BLOB_COUNT,

			// HousekeepingAdvisor
			ID_MIN_WAL_FLUSH_INTERVAL_MS,
			ID_MAX_CLEANUP_WAL_INTERVAL_MS,
			ID_MIN_COMPACTION_INTERVAL_MS,
			ID_PENDING_STORE_ITEM_LIMIT,
			ID_PENDING_STORE_WAL_ITEM_LIMIT,
			ID_COMPACTION_SIZE_MEMORY,
			ID_COMPACTION_SIZE_TREND_MEMORY,
			ID_COMPACTION_STRATEGY,
			ID_COMPACTION_STRATEGY_UPDATE_INTERVAL_MS,
			ID_STCS_MIN_BUCKET_SIZE,

			NUM_IDS
		};

		//! Configuration value types
		enum Type : uint32_t
		{
			TYPE_UNDEFINED,
			TYPE_UINT32,
			TYPE_SIZE,
			TYPE_INTERVAL,
			TYPE_STRING,
			TYPE_BOOL
		};

		//! Information about a configuration value.
		struct Info
		{
			Type			m_type;				//!< Type of value
			const char*		m_id;				//!< String identifier
			const char*		m_default;			//!< Default value
			bool			m_requiresRestart;	//!< Changing this configuration value requires restart
		};

		// IMPORTANT: Order must match the Id enum
		static const Info INFO[] = 
		{
			//                                              | m_type       | m_id                                    | m_default    | m_requiresRestart
			//----------------------------------------------+--------------+-----------------------------------------+--------------+--------------------
			/* ID_WAL_SIZE_LIMIT */                         { TYPE_SIZE,     "wal_size_limit",                         "64MB",        true },
			/* ID_WAL_CONCURRENCY */                        { TYPE_UINT32,   "wal_concurrency",                        "1",           true },
											                				  					                  				     
			/* ID_MAX_RESIDENT_BLOB_SIZE */                 { TYPE_SIZE,     "max_resident_blob_size",                 "1GB",         false },
			/* ID_MAX_RESIDENT_BLOB_COUNT */                { TYPE_SIZE,     "max_resident_blob_count",                "1G",          false },
																		    													     
			/* ID_MIN_WAL_FLUSH_INTERVAL_MS */				{ TYPE_INTERVAL, "min_wal_flush_interval",                 "500ms",       false },
			/* ID_MAX_CLEANUP_WAL_INTERVAL_MS */            { TYPE_INTERVAL, "max_cleanup_wal_interval",               "2m",          false },
			/* ID_MIN_COMPACTION_INTERVAL_MS */             { TYPE_INTERVAL, "min_compaction_interval",                "5s",          false },
			/* ID_PENDING_STORE_ITEM_LIMIT */               { TYPE_UINT32,   "pending_store_item_limit",               "30000",       false },
			/* ID_PENDING_STORE_WAL_ITEM_LIMIT */           { TYPE_UINT32,   "pending_store_wal_item_limit",           "300000",      false },
			/* ID_COMPACTION_SIZE_MEMORY */                 { TYPE_SIZE,     "compaction_size_memory",                 "10",          true },
			/* ID_COMPACTION_SIZE_TREND_MEMORY */           { TYPE_SIZE,     "compaction_size_trend_memory",           "10",          true },
			/* ID_COMPACTION_STRATEGY */                    { TYPE_STRING,   "compaction_strategy",                    "stcs",        true },
			/* ID_COMPACTION_STRATEGY_UPDATE_INTERVAL_MS */ { TYPE_INTERVAL, "compaction_strategy_update_interval",    "10s",		  true },
			/* ID_STCS_MIN_BUCKET_SIZE */                   { TYPE_UINT32,   "stcs_min_bucket_size",                   "4",           true }
		};

		static_assert(sizeof(INFO) == sizeof(Info) * (size_t)NUM_IDS);

		//! Query information about a configuration value
		inline const Info*
		GetInfo(
			uint32_t		aId)
		{
			JELLY_ASSERT(aId < (uint32_t)NUM_IDS);
			return &INFO[aId];
		}

		//! Get configuration value id from string
		inline uint32_t
		StringToId(
			const char*		aString)
		{
			for(uint32_t i = 0; i < (uint32_t)NUM_IDS; i++)
			{
				if(strcmp(INFO[i].m_id, aString) == NULL)
					return i;
			}
			return UINT32_MAX;
		}

	}

}