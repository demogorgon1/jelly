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
		{

		}

		~Backup()
		{

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

		/**
		 * \brief Perform the backup procedure on any thread. When completed, 
		 * pass the backup job back to the node with Node::FinalizeBackup() on
		 * the main thread.
		 */
		void
		Perform()
		{
			if(m_compactionJob.IsSet())
			{
				JELLY_ASSERT(m_compactionStoreId.has_value());
				m_compactionResult = std::make_unique<CompactionResultType>();

				Compaction::Perform<_KeyType, _ItemType>(
					m_host, m_nodeId, m_fileStatsContext, m_compactionStoreId.value(), m_compactionJob, m_compactionResult.get());
			}
			
			char rootPath[1024];
			snprintf(rootPath, sizeof(rootPath), "%s/%s", m_backupPath.c_str(), m_name.c_str());

			if(!std::filesystem::create_directories(rootPath))
				JELLY_FATAL_ERROR("Failed to create path: %s", rootPath);

			for(uint32_t storeId : m_includeStoreIds)
			{
				std::string storePath = m_host->GetStorePath(m_nodeId, storeId);
				std::string storeFileName = StringUtils::GetFileNameFromPath(storePath.c_str());

				char backupStorePath[1024];
				snprintf(backupStorePath, sizeof(backupStorePath), "%s/%s", rootPath, storeFileName.c_str());
				
				std::filesystem::create_hard_link(storePath.c_str(), backupStorePath);
			}

			if(m_prevName.length() > 0)
			{
				char prevPath[1024];
				snprintf(prevPath, sizeof(prevPath), "%s/prev.txt", rootPath);
				FILE* f = fopen(prevPath, "wb");
				JELLY_CHECK(f != NULL, "Failed to open file for output: %s", prevPath);
				fprintf(f, "%s\r\n", m_prevName.c_str());
				fclose(f);
			}
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

	private:

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