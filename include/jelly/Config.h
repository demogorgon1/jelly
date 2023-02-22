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
			// DefaultHost
			ID_COMPRESSION_METHOD,
			ID_COMPRESSION_LEVEL,

			// Node
			ID_WAL_SIZE_LIMIT,
			ID_WAL_CONCURRENCY,
			ID_BACKUP_PATH,
			ID_BACKUP_COMPACTION,
			ID_BACKUP_INCREMENTAL,

			// BlobNode
			ID_MAX_RESIDENT_BLOB_SIZE,
			ID_MAX_RESIDENT_BLOB_COUNT,

			// HousekeepingAdvisor
			ID_MIN_WAL_FLUSH_INTERVAL_MS,
			ID_MAX_CLEANUP_WAL_INTERVAL_MS,
			ID_MIN_COMPACTION_INTERVAL_MS,
			ID_PENDING_STORE_ITEM_LIMIT,
			ID_PENDING_STORE_WAL_ITEM_LIMIT,
			ID_PENDING_STORE_WAL_SIZE_LIMIT,
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
			const char*		m_description;		//!< Description
		};

		// IMPORTANT: Order must match the Id enum
		static const Info INFO[] = 
		{
			//                                              | m_type       | m_id                                    | m_default    | m_requiresRestart
			//----------------------------------------------+--------------+-----------------------------------------+--------------+--------------------
			/* ID_COMPRESSION_METHOD */				        { TYPE_STRING,  "compression_method",                      "zstd",        true,
			   "Specifies how data should be compressed: \"none\" or \"zstd\". Lock node WALs and stores are stream compressed while blob nodes apply "
			   "individual item compression to allow random access." },
			/* ID_COMPRESSION_LEVEL */                      { TYPE_UINT32,  "compression_level",				       "1",           true,
			   "The compression level for the selected compression method. Usually in the range of 1 to 9, but depends on the compression method." },
			//----------------------------------------------+--------------+-----------------------------------------+--------------+--------------------
			/* ID_WAL_SIZE_LIMIT */                         { TYPE_SIZE,     "wal_size_limit",                         "64MB",        false,
			   "If a WAL file reaches this size it will be closed and a new one created." },
			/* ID_WAL_CONCURRENCY */                        { TYPE_UINT32,   "wal_concurrency",                        "1",           true,
			   "Number of WALs to keep open at the same time. These WALs can be flushed in parallel." },
			/* ID_BACKUP_PATH */							{ TYPE_STRING,   "backup_path",							   "backups",	  false,
			   "Path to where backups should be created. This path must point to a directory that is on the same disk volume as the host root due "
			   "to the creation of hard links." },
			/* ID_BACKUP_COMPACTION */                      { TYPE_BOOL,     "backup_compaction",                      "true",        false,
			   "Perform compaction as part of the backup process. If enabled backed up stores will be turned into a single store." },
			/* ID_BACKUP_INCREMENTAL */                     { TYPE_BOOL,     "backup_incremental",                     "true",        false,
			   "Backups only include new stores since last backup. " },
			//----------------------------------------------+--------------+-----------------------------------------+--------------+--------------------
			/* ID_MAX_RESIDENT_BLOB_SIZE */                 { TYPE_SIZE,     "max_resident_blob_size",                 "1GB",         false,
			   "Total size of blobs to keep resident (cached). If blobs exceed this threshold, the oldest ones will be removed from the cache. "
			   "If they're later requested they'll be read from a store on disk." },
			/* ID_MAX_RESIDENT_BLOB_COUNT */                { TYPE_SIZE,     "max_resident_blob_count",                "1G",          false,
			   "Maximum number of blobs to keep resident (cached). If blobs exceed this threshold, the oldest ones will be removed from the cache. "
			   "If they're later requested they'll be read from a store on disk." },
		    //----------------------------------------------+--------------+-----------------------------------------+--------------+--------------------
			/* ID_MIN_WAL_FLUSH_INTERVAL_MS */				{ TYPE_INTERVAL, "min_wal_flush_interval",                 "500ms",       false,
			   "HousekeepingAdvisor: Duration between WAL flushes will never be shorter than this." },
			/* ID_MAX_CLEANUP_WAL_INTERVAL_MS */            { TYPE_INTERVAL, "max_cleanup_wal_interval",               "2m",          false,
			   "HousekeepingAdvisor: Maximum time between WAL cleanups. Usually they'll be triggered after each pending store flush as well. " },
			/* ID_MIN_COMPACTION_INTERVAL_MS */             { TYPE_INTERVAL, "min_compaction_interval",                "5s",          false,
			   "HousekeepingAdvisor: Duration between compactions will never be shorter than this." },
			/* ID_PENDING_STORE_ITEM_LIMIT */               { TYPE_UINT32,   "pending_store_item_limit",               "30000",       false,
			   "HousekeepingAdvisor: If number of unique pending items exceed this threshold a pending store flush will be triggered." },
			/* ID_PENDING_STORE_WAL_ITEM_LIMIT */           { TYPE_UINT32,   "pending_store_wal_item_limit",           "300000",      false,
			   "HousekeepingAdvisor: If number of items written to pending WALs exceed this a pending store flush will be triggered." },
			/* ID_PENDING_STORE_WAL_SIZE_LIMIT */           { TYPE_SIZE,     "pending_store_wal_size_limit",           "128MB",       false,
			   "HousekeepingAdvisor: If total size of all WALs exceed this a pending store flush will be triggered." },
			/* ID_COMPACTION_SIZE_MEMORY */                 { TYPE_SIZE,     "compaction_size_memory",                 "10",          true,
			   "HousekeepingAdvisor: Number of total store size samples to remember back. This is used by the compaction strategy." },
			/* ID_COMPACTION_SIZE_TREND_MEMORY */           { TYPE_SIZE,     "compaction_size_trend_memory",           "10",          true,
			   "HousekeepingAdvisor: Derivative of the store size (velocity) samples. This is used by the compaction strategy." },
			/* ID_COMPACTION_STRATEGY */                    { TYPE_STRING,   "compaction_strategy",                    "stcs",        true,
			   "HousekeepingAdvisor: Compaction strategy to use. Currently \"stcs\" (Size-Tiered Compaction Strategy) is the only option." },
			/* ID_COMPACTION_STRATEGY_UPDATE_INTERVAL_MS */ { TYPE_INTERVAL, "compaction_strategy_update_interval",    "10s",		  false, 
			   "HousekeepingAdvisor: How often to update the compaction strategy." },
			/* ID_STCS_MIN_BUCKET_SIZE */                   { TYPE_SIZE,     "stcs_min_bucket_size",                   "4",           false,
			   "HousekeepingAdvisor: Minimum bucket size for STCS compaction." }
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

		//! Returns number of configuration values available
		inline uint32_t 
		GetCount() 
		{
			return (uint32_t)NUM_IDS;
		}

		//! Get configuration value id from string
		inline uint32_t
		StringToId(
			const char*		aString)
		{
			for(uint32_t i = 0; i < (uint32_t)NUM_IDS; i++)
			{
				if(strcmp(INFO[i].m_id, aString) == 0)
					return i;
			}
			return UINT32_MAX;
		}

	}

}