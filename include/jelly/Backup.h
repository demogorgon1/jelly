#pragma once

#include "Compaction.h"
#include "CompactionJob.h"
#include "CompactionResult.h"
#include "IHost.h"
#include "StringUtils.h"

namespace jelly
{	

	struct FileStatsContext;

	/**
	 * \brief Holds the state of a backup job.
	 * 
	 * In order to do a backup, first call Node::StartBackup() on main thread,
	 * then call Backup::Perform() on any thread, and finally call 
	 * Node::FinalizeBackup() on the main thread.
	 */
	template <typename _KeyType, typename _ItemType>
	class Backup
	{
	public:
		typedef CompactionResult<_KeyType> CompactionResultType;

		Backup(
			IHost*							aHost,
			uint32_t						aNodeId,
			const char*						aBackupPath,
			const char*						aName,
			const char*						aPrevName,
			FileStatsContext*				aFileStatsContext)
			: m_nodeId(aNodeId)
			, m_name(aName)
			, m_prevName(aPrevName)
			, m_backupPath(aBackupPath)
			, m_host(aHost)
			, m_fileStatsContext(aFileStatsContext)
			, m_result(BACKUP_RESULT_NONE)
		{

		}

		~Backup()
		{

		}

		/**
		 * \brief Perform the backup procedure on any thread. When completed, 
		 * pass the backup job back to the node with Node::FinalizeBackup() on
		 * the main thread.
		 */
		bool
		Perform()
		{
			if(m_compactionJob.IsSet())
			{
				JELLY_ASSERT(m_compactionStoreId.has_value());
				m_compactionResult = std::make_unique<CompactionResultType>();

				bool ok = Compaction::Perform<_KeyType, _ItemType>(
					m_host, m_nodeId, m_fileStatsContext, m_compactionStoreId.value(), m_compactionJob, m_compactionResult.get());

				if(!ok)
				{
					m_result = BACKUP_RESULT_FAILED;
					return false;
				}
			}
			
			std::string rootPath = m_backupPath + "/" + m_name;

			// Create the backup directory
			if(!std::filesystem::create_directories(rootPath))
			{
				Log::PrintF(Log::LEVEL_ERROR, "Backup: Failed to create path: %s", rootPath.c_str());
				m_result = BACKUP_RESULT_FAILED;
				return false;
			}

			// Put hard links to store files in it
			for(uint32_t storeId : m_includeStoreIds)
			{
				std::string storePath = m_host->GetStorePath(m_nodeId, storeId);
				std::string storeFileName = StringUtils::GetFileNameFromPath(storePath.c_str());
				std::string backupStorePath = rootPath + "/" + storeFileName;
				
				std::error_code errorCode;
				std::filesystem::create_hard_link(storePath, backupStorePath, errorCode);
				if(errorCode)
				{
					Log::PrintF(Log::LEVEL_ERROR, "Backup: Failed to create hard link: %s (%s -> %s)", errorCode.message().c_str(), storePath.c_str(), backupStorePath.c_str());
					m_result = BACKUP_RESULT_FAILED;
					return false;
				}
			}

			if(m_prevName.length() > 0)
			{
				// Write a little text file with the name of the previous backup that this incremental backup is based on
				std::string prevPath = rootPath + "/prev.txt";
				FILE* f = fopen(prevPath.c_str(), "wb");
				if(f == NULL)
				{
					Log::PrintF(Log::LEVEL_ERROR, "Backup: Failed to open file for output: %s", prevPath.c_str());
					m_result = BACKUP_RESULT_FAILED;
					return false;
				}
				
				fprintf(f, "%s\r\n", m_prevName.c_str());
				fclose(f);
			}

			m_result = BACKUP_RESULT_OK;
			return true;
		}

		void
		SetCompactionJob(
			uint32_t						aCompactionStoreId,
			uint32_t						aOldestStoreId,
			const std::vector<uint32_t>&	aStoreIds)
		{
			m_compactionStoreId = aCompactionStoreId;
			m_compactionJob.m_oldestStoreId = aOldestStoreId;
			m_compactionJob.m_storeIds = aStoreIds;
		}

		void
		SetIncludeStoreIds(
			const std::vector<uint32_t>&	aIncludeStoreIds)
		{
			m_includeStoreIds = aIncludeStoreIds;
		}

		CompactionResultType*
		GetCompactionResult()
		{
			return m_compactionResult.get();
		}

		size_t
		GetIncludedStoreIdCount() const
		{
			return m_includeStoreIds.size();
		}

		bool
		IsIncremental() const
		{
			return m_prevName.length() > 0;
		}

		const std::string&
		GetIncrementalDependency() const
		{
			return m_prevName;
		}

		bool
		HasCompletedOk() const
		{
			JELLY_ASSERT(m_result != BACKUP_RESULT_NONE);
			return m_result == BACKUP_RESULT_OK;
		}

	private:

		enum BackupResult
		{
			BACKUP_RESULT_NONE,
			BACKUP_RESULT_OK,
			BACKUP_RESULT_FAILED
		};

		BackupResult							m_result;
		IHost*									m_host;
		FileStatsContext*						m_fileStatsContext;
		uint32_t								m_nodeId;
		std::string								m_backupPath;
		std::string								m_name;
		std::string								m_prevName;
		CompactionJob							m_compactionJob;
		std::unique_ptr<CompactionResultType>	m_compactionResult;			
		std::optional<uint32_t>					m_compactionStoreId;
		std::vector<uint32_t>					m_includeStoreIds;
	};


}