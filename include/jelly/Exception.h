#pragma once

/**
 * \file Exception.h
 */

namespace jelly
{

	namespace Exception
	{

		/**
		 * Most API calls will return a exception code, which will contain details
		 * about any error that might have occurred. A value of 0 indicates no error.
		 */
		typedef uint32_t Code;

		static const uint32_t ERROR_BIT_MASK				= 0xFFF;
		static const uint32_t ERROR_BIT_OFFSET				= 0;
		static const uint32_t CONTEXT_BIT_MASK				= 0xF;
		static const uint32_t CONTEXT_BIT_OFFSET			= 12;
		static const uint32_t REQUEST_TYPE_BIT_MASK			= 0xF;
		static const uint32_t REQUEST_TYPE_BIT_OFFSET		= 16;
		static const uint32_t LOG_FINGERPRINT_BIT_MASK		= 0xFFF;
		static const uint32_t LOG_FINGERPRINT_BIT_OFFSET	= 20;

		/**
		 * Error contexts are encoded into result codes and tell during which user API call
		 * the error occured.
		 */
		enum Context : uint32_t
		{
			CONTEXT_NONE,							//!< No context (or unknown)
			CONTEXT_BACKUP_PERFORM,					//!< While performing a backup
			CONTEXT_BLOB_NODE_INIT,					//!< While restoring a blob node
			CONTEXT_LOCK_NODE_INIT,					//!< While restoring a lock node
			CONTEXT_NODE_START_BACKUP,				//!< While starting a backup
			CONTEXT_NODE_FINALIZE_BACKUP,			//!< While finalazing a backup
			CONTEXT_NODE_PROCESS_REPLICATION,		//!< While processing replication data from another node
			CONTEXT_NODE_PROCESS_REQUESTS,			//!< While processing requests
			CONTEXT_NODE_FLUSH_PENDING_WAL,			//!< While flushing pending WALs
			CONTEXT_NODE_FLUSH_PENDING_STORE,		//!< While flushing a pending store
			CONTEXT_NODE_CLEANUP_WALS,				//!< While cleaning up WALs
			CONTEXT_NODE_PERFORM_COMPACTION,		//!< While performing (minor) compaction
			CONTEXT_NODE_PERFORM_MAJOR_COMPECTION,	//!< While performing major compaction
			CONTEXT_NODE_APPLY_COMPACTION_RESULT,	//!< While applying compaction results

			NUM_CONTEXTS
		};

		/**
		 * Request types are encoded into result codes as well. This tells the application
		 * during which type of request the error occurred.
		 */
		enum RequestType : uint32_t
		{
			REQUEST_TYPE_NONE,						//!< Error didn't happen during a request
			REQUEST_TYPE_BLOB_NODE_SET,				//!< While performing a blob node set request
			REQUEST_TYPE_BLOB_NODE_GET,				//!< While performing a blob node get request
			REQUEST_TYPE_BLOB_NODE_DELETE,			//!< While performing a blob node delete request
			REQUEST_TYPE_LOCK_NODE_LOCK,			//!< While performing a lock node lock request
			REQUEST_TYPE_LOCK_NODE_UNLOCK,			//!< While performing a lock node unlock request
			REQUEST_TYPE_LOCK_NODE_DELETE,			//!< While performing a lock node delete request

			NUM_REQUEST_TYPES
		};

		/**
		 * Error categories. During testing this can be used to simulate errors in different 
		 * categories.
		 */
		enum Category : uint32_t
		{
			CATEGORY_NONE,							//!< No category
			CATEGORY_DISK_READ,						//!< Disk read error
			CATEGORY_DISK_WRITE,					//!< Disk write error
			CATEGORY_DISK_AVAILABLE_SPACE,			//!< Error due to low available disk space
			CATEGORY_DISK_CREATE_FILE,				//!< Failed to create a file or directory
			CATEGORY_DISK_OPEN_FILE,				//!< Failed to open a file
			CATEGORY_BACKUP,						//!< Backup related error
			CATEGORY_COMPACTION,					//!< Compaction related error
			CATEGORY_COMPRESSION,					//!< Compression related error
			CATEGORY_DECOMPRESSION,					//!< Decompression related error
			CATEGORY_SYSTEM,						//!< System error
			CATEGORY_CONFIGURATION,					//!< Configuration error

			NUM_CATEGORIES
		};

		/**
		 * All individual error conditions have their own unique error code. This helps
		 * figuring out exactly what went wrong when you see a result code. It also helps testing
		 * exercise as many error control paths as possible by simulating specific errors.
		 */
		enum Error : uint32_t
		{
			ERROR_NONE,
			ERROR_BACKUP_FAILED_COMPACTION,
			ERROR_BACKUP_FAILED_TO_CREATE_DIRECTORY,
			ERROR_BACKUP_FAILED_TO_CREATE_HARD_LINK,
			ERROR_BACKUP_FAILED_TO_CREATE_PREV_FILE,
			ERROR_COMPACTION_FAILED_TO_OPEN_OUTPUT_STORE,
			ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN,
			ERROR_FILE_WRITE_STREAM_FAILED_TO_FLUSH,
			ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE,
			ERROR_FILE_READ_STREAM_FAILED_TO_OPEN,
			ERROR_FILE_READ_STREAM_FAILED_TO_READ_HEADER,
			ERROR_FILE_READ_STREAM_HEADER_MISMATCH,
			ERROR_FILE_READ_STREAM_FAILED_TO_READ,
			ERROR_FILE_READ_RANDOM_FAILED_TO_OPEN,
			ERROR_FILE_READ_RANDOM_FAILED_TO_READ_HEADER,
			ERROR_FILE_READ_RANDOM_HEADER_MISMATCH,
			ERROR_FILE_READ_RANDOM_FAILED_TO_READ,
			ERROR_FILE_LOCK_ALREADY_LOCKED,
			ERROR_FILE_LOCK_FAILED_TO_OPEN,
			ERROR_ZSTD_STREAM_FAILED_TO_CREATE_COMPRESSOR,
			ERROR_ZSTD_STREAM_FAILED_TO_READ,
			ERROR_ZSTD_STREAM_FAILED_TO_CREATE_DECOMPRESSOR,
			ERROR_ZSTD_STREAM_FAILED_TO_FLUSH,
			ERROR_ZSTD_STREAM_FAILED_TO_WRITE,
			ERROR_ZSTD_BUFFER_TOO_LARGE_TO_COMPRESS,
			ERROR_ZSTD_BUFFER_COMPRESSION_FAILED,
			ERROR_ZSTD_BUFFER_INVALID,
			ERROR_ZSTD_BUFFER_TOO_LARGE_TO_UNCOMPRESS,
			ERROR_ZSTD_BUFFER_DECOMPRESSION_FAILED,
			ERROR_PATH_TOO_LONG,
			ERROR_INVALID_CHARACTER_IN_FILE_PREFIX,
			ERROR_STORE_WRITER_RENAME_FAILED,
			ERROR_INVALID_UINT32,
			ERROR_INVALID_SIZE,
			ERROR_INVALID_BOOL,
			ERROR_INVALID_INTERVAL,
			ERROR_INTERVAL_OUT_OF_BOUNDS,
			ERROR_SIZE_STRING_TOO_LONG,
			ERROR_PATH_IS_EMPTY,
			ERROR_FAILED_TO_GET_MEMORY_INFO,
			ERROR_INVALID_COMPRESSION_METHOD,
			ERROR_FAILED_TO_GET_AVAILABLE_DISK_SPACE,
			ERROR_FAILED_TO_ENUMERATE_FILES,
			ERROR_FAILED_TO_GET_STORE_INFO,
			ERROR_FAILED_TO_DELETE_WAL,
			ERROR_FAILED_TO_GET_LATEST_BACKUP,
			ERROR_FAILED_TO_GET_BLOB_READER,
			ERROR_FAILED_TO_GET_TOTAL_DISK_USAGE,
			ERROR_FAILED_TO_DELETE_ALL_FILES,
			ERROR_NOTHING_TO_BACKUP,
			ERROR_NOT_ENOUGH_AVAILABLE_SPACE_FOR_BACKUP,
			ERROR_MAJOR_COMPACTION_IN_PROGRESS,
			ERROR_COMPACTION_IN_PROGRESS,

			NUM_ERRORS
		};

		static_assert((uint32_t)NUM_CONTEXTS <= CONTEXT_BIT_MASK); 
		static_assert((uint32_t)NUM_REQUEST_TYPES <= REQUEST_TYPE_BIT_MASK); 
		static_assert((uint32_t)NUM_ERRORS <= ERROR_BIT_MASK); 

		// IMPORTANT: Must match Context enum
		static constexpr const char* CONTEXT_STRINGS[] =
		{
			"NONE",
			"BACKUP_PERFORM",
			"BLOB_NODE_INIT",
			"LOCK_NODE_INIT",
			"NODE_START_BACKUP",
			"NODE_FINALIZE_BACKUP",
			"NODE_PROCESS_REPLICATION",
			"NODE_PROCESS_REQUESTS",
			"NODE_FLUSH_PENDING_WAL",
			"NODE_FLUSH_PENDING_STORE",
			"NODE_CLEANUP_WALS",
			"NODE_PERFORM_COMPACTION",
			"NODE_PERFORM_MAJOR_COMPACTION",
			"NODE_APPLY_COMPACTION_RESULT"
		};

		// IMPORTANT: Must match RequestType enum
		static constexpr const char* REQUEST_TYPE_STRINGS[] =
		{
			"NONE",
			"BLOB_NODE_SET",
			"BLOB_NODE_GET",
			"BLOB_NODE_DELETE",
			"LOCK_NODE_LOCK",
			"LOCK_NODE_UNLOCK",
			"LOCK_NODE_DELETE"
		};

		// IMPORTANT: Must match Category enum
		static constexpr const char* CATEGORY_STRINGS[] =
		{
			"NONE",
			"DISK_READ",
			"DISK_WRITE",
			"DISK_AVAILABLE_SPACE",
			"DISK_CREATE_FILE",
			"DISK_OPEN_FILE",
			"BACKUP",
			"COMPACTION",
			"COMPRESSION",
			"DECOMPRESSION",
			"SYSTEM",
			"CONFIGURATION"
		};

		/**
		 * Information by an entry in the Error enum. Retrieve with GetErrorInfo().
		 */
		struct ErrorInfo
		{
			const char*					m_string;		//!< String id
			Category					m_category;		//!< Error category
			const char*					m_description;	//!< Error description for documentation
		};

		// IMPORTANT: Must match Error enum
		static constexpr ErrorInfo ERROR_INFO[] =
		{
			{ "NONE",										CATEGORY_NONE,					"No error." },
			{ "BACKUP_FAILED_COMPACTION",					CATEGORY_BACKUP,				"Something went wrong during compacting store to be backed up." },
			{ "BACKUP_FAILED_TO_CREATE_DIRECTORY",			CATEGORY_BACKUP,				"Failed to create backup directory." },
			{ "BACKUP_FAILED_TO_CREATE_HARD_LINK",			CATEGORY_BACKUP,				"Failed to create hard link to backed up store." },
			{ "BACKUP_FAILED_TO_CREATE_PREV_FILE",			CATEGORY_BACKUP,				"Failed to create a file to store name of previous backup in incremental backup chain." },
			{ "COMPACTION_FAILED_TO_OPEN_OUTPUT_STORE",		CATEGORY_COMPACTION,			"Failed to create compacted store." },
			{ "FILE_WRITE_STREAM_FAILED_TO_OPEN",			CATEGORY_DISK_CREATE_FILE,		"Failed to create a file stream for writing." },
			{ "FILE_WRITE_STREAM_FAILED_TO_FLUSH",			CATEGORY_DISK_WRITE,			"Failed to flush file write stream." },
			{ "FILE_WRITE_STREAM_FAILED_TO_WRITE",			CATEGORY_DISK_WRITE,			"Failed to write data to file write stream." },
			{ "FILE_READ_STREAM_FAILED_TO_OPEN",			CATEGORY_DISK_OPEN_FILE,		"Failed to open file read stream." },
			{ "FILE_READ_STREAM_FAILED_TO_READ_HEADER",		CATEGORY_DISK_READ,				"Failed to read header from file read stream." },
			{ "FILE_READ_STREAM_HEADER_MISMATCH",			CATEGORY_DISK_READ,				"File read stream opened has a bad header." },
			{ "FILE_READ_STREAM_FAILED_TO_READ",			CATEGORY_DISK_READ,				"Failed to read data from file read stream." },
			{ "FILE_READ_RANDOM_FAILED_TO_OPEN",			CATEGORY_DISK_OPEN_FILE,		"Failed to open file for random access." },
			{ "FILE_READ_RANDOM_FAILED_TO_READ_HEADER",		CATEGORY_DISK_READ,				"Failed to read header from random access file." },
			{ "FILE_READ_RANDOM_HEADER_MISMATCH",			CATEGORY_DISK_READ,				"Random access file opened has a bad header." },
			{ "FILE_READ_RANDOM_FAILED_TO_READ",			CATEGORY_DISK_READ,				"Failed to read data from random access file." },
			{ "FILE_LOCK_ALREADY_LOCKED",					CATEGORY_CONFIGURATION,			"Named lock has already been acquired. Probably some other node process already running with same node id." },
			{ "FILE_LOCK_FAILED_TO_OPEN",					CATEGORY_SYSTEM,				"Failed to open named lock." },
			{ "ZSTD_STREAM_FAILED_TO_CREATE_COMPRESSOR",	CATEGORY_COMPRESSION,			"Failed to initialize ZSTD stream compression." },
			{ "ZSTD_STREAM_FAILED_TO_READ",					CATEGORY_DECOMPRESSION,			"Failed to read from ZSTD stream." },
			{ "ZSTD_STREAM_FAILED_TO_CREATE_DECOMPRESSOR",	CATEGORY_DECOMPRESSION,			"Failed to initialize ZSTD stream decompression." },
			{ "ZSTD_STREAM_FAILED_TO_FLUSH",				CATEGORY_COMPRESSION,			"Failed to flush ZSTD stream compression." },
			{ "ZSTD_STREAM_FAILED_TO_WRITE",				CATEGORY_COMPRESSION,			"Failed to write to ZSTD stream." },
			{ "ZSTD_BUFFER_TOO_LARGE_TO_COMPRESS",			CATEGORY_COMPRESSION,			"Trying to ZSTD compress a too large buffer." },
			{ "ZSTD_BUFFER_COMPRESSION_FAILED",				CATEGORY_COMPRESSION,			"Failed to ZSTD compress a buffer." },
			{ "ZSTD_BUFFER_INVALID",						CATEGORY_DECOMPRESSION,			"Not a valid ZSTD buffer." },
			{ "ZSTD_BUFFER_TOO_LARGE_TO_UNCOMPRESS",		CATEGORY_DECOMPRESSION,			"Unable to decompress ZSTD buffer as result would be too large. Buffer might be corrupted or not a ZSTD buffer." },
			{ "ZSTD_BUFFER_DECOMPRESSION_FAILED",			CATEGORY_DECOMPRESSION,			"Failed to ZSTD decompress a buffer." },
			{ "PATH_TOO_LONG",								CATEGORY_CONFIGURATION,			"Store or WAL path is too long." },
			{ "INVALID_CHARACTER_IN_FILE_PREFIX",			CATEGORY_CONFIGURATION,			"Invalid character encountered in file prefix." },
			{ "STORE_WRITER_RENAME_FAILED",					CATEGORY_DISK_CREATE_FILE,		"Failed to rename created store from temporary to target name." },
			{ "INVALID_UINT32",								CATEGORY_CONFIGURATION,			"Failed to parse invalid uint32 string." },
			{ "INVALID_SIZE",								CATEGORY_CONFIGURATION,			"Failed to parse invalid size string." },
			{ "INVALID_BOOL",								CATEGORY_CONFIGURATION,			"Failed to parse invalid bool string." },
			{ "INVALID_INTERVAL",							CATEGORY_CONFIGURATION,			"Failed to parse invalid interval string." },
			{ "INTERVAL_OUT_OF_BOUNDS",						CATEGORY_CONFIGURATION,			"Parsed interval string is out of bounts. Must be equal to or shorter than 2^32 seconds." },
			{ "SIZE_STRING_TOO_LONG",						CATEGORY_CONFIGURATION,			"Unable to create size string as it would be too long." },
			{ "PATH_IS_EMPTY",								CATEGORY_CONFIGURATION,			"Unable to get file name from path as path is empty." },
			{ "FAILED_TO_GET_MEMORY_INFO",					CATEGORY_SYSTEM,				"Failed to get current memory usage information." },
			{ "INVALID_COMPRESSION_METHOD",					CATEGORY_CONFIGURATION,			"Invalid compression method." },
			{ "FAILED_TO_GET_AVAILABLE_DISK_SPACE",			CATEGORY_SYSTEM,				"Failed to get available disk space in path." },
			{ "FAILED_TO_ENUMERATE_FILES",					CATEGORY_SYSTEM,				"Error occured while finding store and WAL files in root directory." },
			{ "FAILED_TO_GET_STORE_INFO",					CATEGORY_SYSTEM,				"Failed to enumerate information about stores in root directory." },
			{ "FAILED_TO_DELETE_WAL",						CATEGORY_SYSTEM,				"Failed to delete WAL from root directory." },
			{ "FAILED_TO_GET_LATEST_BACKUP",				CATEGORY_SYSTEM,				"Failed to get information about the latest backup." },
			{ "FAILED_TO_GET_BLOB_READER",					CATEGORY_DISK_OPEN_FILE,		"Blob was supposed to be in a store that doesn't exist." },
			{ "FAILED_TO_GET_TOTAL_DISK_USAGE",				CATEGORY_SYSTEM,				"Failed to enumerate all files in root directory to determine total disk usage." },
			{ "FAILED_TO_DELETE_ALL_FILES",					CATEGORY_SYSTEM,				"Host was requested to delete all files, but was unable to search for files to delete in the root directory." },
			{ "NOTHING_TO_BACKUP",							CATEGORY_BACKUP,				"No data available for backup." },
			{ "NOT_ENOUGH_AVAILABLE_SPACE_FOR_BACKUP",		CATEGORY_DISK_AVAILABLE_SPACE,	"Not enough available space to start backup." },
			{ "MAJOR_COMPACTION_IN_PROGRESS",				CATEGORY_COMPACTION,			"Major compaction already in progress." },
			{ "COMPACTION_IN_PROGRESS",						CATEGORY_COMPACTION,			"Tried to perform compaction on a store that is currently being compacted." }
		};

		static_assert(sizeof(CATEGORY_STRINGS) / sizeof(const char*) == NUM_CATEGORIES);
		static_assert(sizeof(REQUEST_TYPE_STRINGS) / sizeof(const char*) == NUM_REQUEST_TYPES);
		static_assert(sizeof(CONTEXT_STRINGS) / sizeof(const char*) == NUM_CONTEXTS);
		static_assert(sizeof(ERROR_INFO) / sizeof(ErrorInfo) == NUM_ERRORS);

		/**
		 * Make a exception code from: error, context, request type, and log fingerprint. The log fingerprint
		 * is a (somewhat) unique number that can be used to associate a log line with a specific result code.
		 */
		inline constexpr uint32_t
		MakeExceptionCode(
			uint32_t		aError,
			uint32_t		aContext,
			uint32_t		aRequestType,
			uint32_t		aLogFingerprint) noexcept
		{
			return ((aError & ERROR_BIT_MASK) << ERROR_BIT_OFFSET)
				| ((aContext & CONTEXT_BIT_MASK) << CONTEXT_BIT_OFFSET)
				| ((aRequestType & REQUEST_TYPE_BIT_MASK) << REQUEST_TYPE_BIT_OFFSET)
				| ((aLogFingerprint & LOG_FINGERPRINT_BIT_MASK) << LOG_FINGERPRINT_BIT_OFFSET);
		}

		/**
		 * Extract error from exception code.
		 */
		inline constexpr Error
		GetExceptionCodeError(
			uint32_t		aResultCode) noexcept
		{
			return (Error)((aResultCode >> ERROR_BIT_OFFSET) & ERROR_BIT_MASK);
		}

		/**
		 * Extract context from exception code.
		 */
		inline constexpr Context
		GetExceptionCodeContext(
			uint32_t		aResultCode) noexcept
		{
			return (Context)((aResultCode >> CONTEXT_BIT_OFFSET) & CONTEXT_BIT_MASK);
		}

		/**
		 * Extract request type from exception code.
		 */
		inline constexpr RequestType
		GetExceptionCodeRequestType(
			uint32_t		aResultCode) noexcept
		{
			return (RequestType)((aResultCode >> REQUEST_TYPE_BIT_OFFSET) & REQUEST_TYPE_BIT_MASK);
		}

		/**
		 * Extract log fingerprint from exception code.
		 */
		inline constexpr uint32_t
		GetExceptionCodeLogFingerprint(
			uint32_t		aResultCode) noexcept
		{
			return (aResultCode >> LOG_FINGERPRINT_BIT_OFFSET) & LOG_FINGERPRINT_BIT_MASK;
		}

		/** 
		* Get information (description, category) about an error.
		*/
		inline constexpr const ErrorInfo*
		GetErrorInfo(
			uint32_t		aError) noexcept
		{
			return &ERROR_INFO[aError];
		}

		/**
		 * Get category as a string.
		 */
		inline constexpr const char*
		GetCategoryString(
			uint32_t		aId) noexcept
		{
			return CATEGORY_STRINGS[aId];
		}

		/**
		 * Get request type as a string.
		 */
		inline constexpr const char*
		GetRequestTypeString(
			uint32_t		aId) noexcept
		{
			return REQUEST_TYPE_STRINGS[aId];
		}

		/** 
		 * Get context as a string.
		 */
		inline constexpr const char*
		GetContextString(
			uint32_t		aId) noexcept
		{
			return CONTEXT_STRINGS[aId];
		}

	}

}