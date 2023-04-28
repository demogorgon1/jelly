#include <jelly/Base.h>

#include <jelly/Compression.h>
#include <jelly/ConfigProxy.h>
#include <jelly/DefaultConfigSource.h>
#include <jelly/DefaultHost.h>
#include <jelly/ErrorUtils.h>
#include <jelly/File.h>
#include <jelly/FileHeader.h>
#include <jelly/IStats.h>
#include <jelly/StringUtils.h>
#include <jelly/ZstdCompression.h>

#include "FileStreamReader.h"
#include "PathUtils.h"
#include "Stats.h"
#include "StoreBlobReader.h"
#include "StoreWriter.h"
#include "SystemUtils.h"
#include "WALWriter.h"

namespace jelly
{

	DefaultHost::DefaultHost(
		const char*					aRoot,
		const char*					aFilePrefix,
		const IConfigSource*		aConfigSource,
		const Stat::Info*			aExtraApplicationStats,
		uint32_t					aExtraApplicationStatsCount)
		: m_root(aRoot)
		, m_filePrefix(aFilePrefix != NULL ? aFilePrefix : "")
		, m_configSource(aConfigSource)
	{
		// If root directory doesn't exist, create it
		std::filesystem::create_directories(aRoot);

		{
			// Make sure that no other host operates in the same root
			std::string fileLockPath = StringUtils::Format("%s/%shost.lck", aRoot, aFilePrefix);
			m_fileLock = std::make_unique<File>(nullptr, fileLockPath.c_str(), File::MODE_MUTEX, FileHeader(FileHeader::TYPE_NONE));
		}

		if(m_configSource == NULL)
		{
			m_defaultConfigSource = std::make_unique<DefaultConfigSource>();
			m_configSource = m_defaultConfigSource.get();
		}

		m_config = std::make_unique<ConfigProxy>(m_configSource);
		m_stats = std::make_unique<Stats>(Stats::ExtraApplicationStats(aExtraApplicationStats, aExtraApplicationStatsCount));

		// Initialize compression
		{
			const char* compressionMethod = m_config->GetString(Config::ID_COMPRESSION_METHOD);
			if(strcmp(compressionMethod, "zstd") == 0)
			{
				#if defined(JELLY_ZSTD)
					m_compressionProvider = std::make_unique<ZstdCompression>(m_config->GetUInt32(Config::ID_COMPRESSION_LEVEL)); 				
				#endif
			}
			else if(strcmp(compressionMethod, "none") == 0)
			{
				// Do nothing
			}
			else
			{
				JELLY_FAIL(Exception::ERROR_INVALID_COMPRESSION_METHOD, "Method=%s", compressionMethod);
			}
		}

		// Initialize store manager
		{
			FileHeader fileHeader(FileHeader::TYPE_STORE);

			if (m_compressionProvider != NULL)
			{
				fileHeader.m_flags |= FileHeader::FLAG_ITEM_COMPRESSION;
				fileHeader.m_compressionId = m_compressionProvider->GetId();
			}

			m_storeManager = std::make_unique<StoreManager>(aRoot, aFilePrefix, fileHeader);
		}
	}
	
	DefaultHost::~DefaultHost()
	{

	}

	void				
	DefaultHost::PollSystemStats()
	{
		// Determine total size on disk
		{
			size_t totalFileSizes[PathUtils::NUM_FILES_TYPES];
			memset(totalFileSizes, 0, sizeof(totalFileSizes));

			std::error_code errorCode;
			std::filesystem::directory_iterator it(m_root, errorCode);
			JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_GET_TOTAL_DISK_USAGE, "Path=%s;Msg=%s", m_root.c_str(), errorCode.message().c_str());

			for (const std::filesystem::directory_entry& entry : it)
			{
				PathUtils::FileType fileType;
				uint32_t id;
				uint32_t nodeId;
				if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), m_filePrefix.c_str(), fileType, nodeId, id))
				{
					JELLY_ASSERT(fileType < PathUtils::NUM_FILES_TYPES);
					totalFileSizes[fileType] += (size_t)entry.file_size();
				}
			}

			m_stats->Emit(Stat::ID_TOTAL_HOST_STORE_SIZE, totalFileSizes[PathUtils::FILE_TYPE_STORE]);
			m_stats->Emit(Stat::ID_TOTAL_HOST_WAL_SIZE, totalFileSizes[PathUtils::FILE_TYPE_WAL]);
		}

		m_stats->Emit(Stat::ID_AVAILABLE_DISK_SPACE, GetAvailableDiskSpace());

		SystemUtils::MemoryInfo memoryInfo = SystemUtils::GetMemoryInfo();
		m_stats->Emit(Stat::ID_MEMORY_USAGE, memoryInfo.m_usage);
	}

	void		
	DefaultHost::DeleteAllFiles(
		uint32_t					aNodeId)
	{			
		m_storeManager->CloseAll();

		std::error_code errorCode;
		std::filesystem::directory_iterator it(m_root, errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_DELETE_ALL_FILES, "Path=%s;NodeId=%u;Msg=%s", m_root.c_str(), aNodeId, errorCode.message().c_str());

		for (const std::filesystem::directory_entry& entry : it)
		{
			PathUtils::FileType fileType;
			uint32_t id;
			uint32_t nodeId;
			if(entry.is_regular_file() && PathUtils::ParsePath(entry.path(), m_filePrefix.c_str(), fileType, nodeId, id))
			{
				if(aNodeId == UINT32_MAX || nodeId == aNodeId)
					std::filesystem::remove(entry.path());
			}
		}
	}

	//---------------------------------------------------------------

	IStats* 
	DefaultHost::GetStats() 
	{
		return m_stats.get();
	}

	const IConfigSource* 
	DefaultHost::GetConfigSource() 
	{
		return m_configSource;
	}

	Compression::IProvider* 
	DefaultHost::GetCompressionProvider() 
	{
		return m_compressionProvider.get();
	}

	uint64_t	
	DefaultHost::GetTimeStamp() 
	{
		return (uint64_t)time(NULL);
	}

	size_t					
	DefaultHost::GetAvailableDiskSpace() 
	{
		std::error_code errorCode;
		std::filesystem::space_info si = std::filesystem::space(m_root, errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_GET_AVAILABLE_DISK_SPACE, "Path=%s;Msg=%s", m_root.c_str(), errorCode.message().c_str());
		return (size_t)si.available;;
	}

	void		
	DefaultHost::EnumerateFiles(
		uint32_t					aNodeId,
		std::vector<uint32_t>&		aOutWriteAheadLogIds,
		std::vector<uint32_t>&		aOutStoreIds) 
	{
		std::error_code errorCode;
		std::filesystem::directory_iterator it(m_root, errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_ENUMERATE_FILES, "NodeId=%u;Path=%s;Msg=%s", aNodeId, m_root.c_str(), errorCode.message().c_str());

		for (const std::filesystem::directory_entry& entry : it)
		{
			PathUtils::FileType fileType;
			uint32_t id;
			uint32_t nodeId;
			if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), m_filePrefix.c_str(), fileType, nodeId, id))
			{
				if (nodeId == aNodeId)
				{
					switch (fileType)
					{
					case PathUtils::FILE_TYPE_STORE:	aOutStoreIds.push_back(id); break;
					case PathUtils::FILE_TYPE_WAL:		aOutWriteAheadLogIds.push_back(id); break;
					default:							JELLY_ASSERT(false);
					}
				}
			}
		}

		std::sort(aOutStoreIds.begin(), aOutStoreIds.end());
		std::sort(aOutWriteAheadLogIds.begin(), aOutWriteAheadLogIds.end());
	}

	void		
	DefaultHost::GetStoreInfo(
		uint32_t					aNodeId,
		std::vector<StoreInfo>&		aOut) 
	{
		std::error_code errorCode;
		std::filesystem::directory_iterator it(m_root, errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_GET_STORE_INFO, "NodeId=%u;Path=%s;Msg=%s", aNodeId, m_root.c_str(), errorCode.message().c_str());

		for (const std::filesystem::directory_entry& entry : it)
		{
			PathUtils::FileType fileType;
			uint32_t nodeId;
			uint32_t id;
			if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), m_filePrefix.c_str(), fileType, nodeId, id))
			{	
				if(nodeId == aNodeId && fileType == PathUtils::FILE_TYPE_STORE)
				{
					StoreInfo t;
					t.m_id = id;
					t.m_size = (size_t)entry.file_size();
					aOut.push_back(t);
				}
			}
		}

		std::sort(aOut.begin(), aOut.end());
	}

	IFileStreamReader*
	DefaultHost::ReadWALStream(
		uint32_t					aNodeId,
		uint32_t					aId,
		bool						aUseStreamingCompression,
		FileStatsContext*			aFileStatsContext)
	{
		FileHeader fileHeader(FileHeader::TYPE_WAL);

		if(aUseStreamingCompression && m_compressionProvider != NULL)
		{
			fileHeader.m_flags |= FileHeader::FLAG_STREAM_COMPRESSION;
			fileHeader.m_compressionId = m_compressionProvider->GetId();
		}

		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider && aUseStreamingCompression ? m_compressionProvider->CreateStreamDecompressor() : NULL,
			aFileStatsContext,
			fileHeader));

		if(!f->IsValid())
			return NULL;

		return f.release();
	}
	
	IWALWriter*
	DefaultHost::CreateWAL(
		uint32_t					aNodeId,
		uint32_t					aId,
		bool						aUseStreamingCompression,
		FileStatsContext*			aFileStatsContext) 
	{
		FileHeader fileHeader(FileHeader::TYPE_WAL);

		if (aUseStreamingCompression && m_compressionProvider != NULL)
		{
			fileHeader.m_flags |= FileHeader::FLAG_STREAM_COMPRESSION;
			fileHeader.m_compressionId = m_compressionProvider->GetId();
		}

		std::unique_ptr<WALWriter> f(new WALWriter(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider && aUseStreamingCompression ? m_compressionProvider.get() : NULL,
			aFileStatsContext,
			fileHeader));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}
	
	void		
	DefaultHost::DeleteWAL(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		std::error_code errorCode;
		std::filesystem::remove(PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(), errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_DELETE_WAL, "NodeId=%u;Id=%u;Msg=%s", aNodeId, aNodeId, errorCode.message().c_str());
	}
	
	IFileStreamReader*
	DefaultHost::ReadStoreStream(
		uint32_t					aNodeId,
		uint32_t					aId,
		FileStatsContext*			aFileStatsContext) 
	{
		FileHeader fileHeader(FileHeader::TYPE_STORE);

		if (m_compressionProvider != NULL)
		{
			fileHeader.m_flags |= FileHeader::FLAG_ITEM_COMPRESSION;
			fileHeader.m_compressionId = m_compressionProvider->GetId();
		}

		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId).c_str(),
			NULL,
			aFileStatsContext,
			fileHeader));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}

	IStoreBlobReader* 
	DefaultHost::GetStoreBlobReader(
		uint32_t					aNodeId,
		uint32_t					aId,
		FileStatsContext*			aFileStatsContext)
	{
		return m_storeManager->GetStoreBlobReader(aNodeId, aId, aFileStatsContext);
	}
	
	IStoreWriter*
	DefaultHost::CreateStore(
		uint32_t					aNodeId,
		uint32_t					aId,
		FileStatsContext*			aFileStatsContext)
	{
		std::string targetPath = PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId);
		std::string tempPath = targetPath + ".tmp";

		FileHeader fileHeader(FileHeader::TYPE_STORE);

		if(m_compressionProvider != NULL)
		{
			fileHeader.m_flags |= FileHeader::FLAG_ITEM_COMPRESSION;
			fileHeader.m_compressionId = m_compressionProvider->GetId();
		}

		std::unique_ptr<StoreWriter> f(new StoreWriter(
			targetPath.c_str(),
			tempPath.c_str(),
			aFileStatsContext,
			fileHeader));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}

	void		
	DefaultHost::DeleteStore(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		m_storeManager->DeleteStore(aNodeId, aId);
	}

	File* 
	DefaultHost::CreateNodeLock(
		uint32_t					aNodeId) 
	{
		return new File(
			NULL, 
			StringUtils::Format("%s/%snode-%u.lck", m_root.c_str(), m_filePrefix.c_str(), aNodeId).c_str(), 
			File::MODE_MUTEX,
			FileHeader(FileHeader::TYPE_NONE));
	}

	bool
	DefaultHost::GetLatestBackupInfo(
		uint32_t					aNodeId,
		const char*					aBackupPath,
		std::string&				aOutName,
		uint32_t&					aOutLatestStoreId) 
	{
		aOutLatestStoreId = UINT32_MAX;

		if(!std::filesystem::exists(aBackupPath))
			return false;

		std::error_code errorCode;
		std::filesystem::directory_iterator it(aBackupPath, errorCode);
		JELLY_CHECK(!errorCode, Exception::ERROR_FAILED_TO_GET_LATEST_BACKUP, "NodeId=%u;Path=%s;Msg=%s", aNodeId, aBackupPath, errorCode.message().c_str());

		for (const std::filesystem::directory_entry& entry : it)
		{
			// Each backup has its own directory in the backup path
			if(entry.is_directory())
			{
				for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator(entry.path()))
				{
					PathUtils::FileType fileType;
					uint32_t id;
					uint32_t nodeId;
					if (file.is_regular_file() && PathUtils::ParsePath(file.path(), m_filePrefix.c_str(), fileType, nodeId, id))
					{
						if (nodeId == aNodeId && fileType == PathUtils::FILE_TYPE_STORE)
						{
							if(aOutLatestStoreId == UINT32_MAX || id > aOutLatestStoreId)
							{
								aOutLatestStoreId = id;
								aOutName = entry.path().filename().string();
							}
						}
					}
				}
			}
		}

		return aOutLatestStoreId != UINT32_MAX;
	}

	std::string				
	DefaultHost::GetStorePath(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		return PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId);
	}

}
