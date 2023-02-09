#include <jelly/Base.h>

#include <jelly/Compression.h>
#include <jelly/ConfigProxy.h>
#include <jelly/DefaultConfigSource.h>
#include <jelly/DefaultHost.h>
#include <jelly/ErrorUtils.h>
#include <jelly/File.h>
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
		IConfigSource*				aConfigSource,
		const Stat::Info*			aExtraApplicationStats,
		uint32_t					aExtraApplicationStatsCount)
		: m_root(aRoot)
		, m_filePrefix(aFilePrefix)
		, m_configSource(aConfigSource)
	{
		{
			// Make sure that no other host operates in the same root
			std::string fileLockPath = StringUtils::Format("%s/%shost.lck", aRoot, aFilePrefix);
			m_fileLock = std::make_unique<File>(nullptr, fileLockPath.c_str(), File::MODE_MUTEX);
		}

		if(m_configSource == NULL)
		{
			m_defaultConfigSource = std::make_unique<DefaultConfigSource>();
			m_configSource = m_defaultConfigSource.get();
		}

		m_config = std::make_unique<ConfigProxy>(m_configSource);

		m_storeManager = std::make_unique<StoreManager>(aRoot, aFilePrefix);
		m_stats = std::make_unique<Stats>(Stats::ExtraApplicationStats(aExtraApplicationStats, aExtraApplicationStatsCount));

		{
			const char* compressionMethod = m_config->GetString(Config::ID_COMPRESSION_METHOD);
			if(strcmp(compressionMethod, "zstd") == 0)
			{
				#if defined(JELLY_ZSTD)
					m_compressionProvider = std::make_unique<ZstdCompression>(m_config->GetUInt32(Config::ID_COMPRESSION_LEVEL)); 				
				#endif
			}
			else
			{
				JELLY_FATAL_ERROR("Invalid compression method: %s", compressionMethod);
			}
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
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
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

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
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

	IConfigSource* 
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
		size_t available = 0;

		try
		{
			std::filesystem::space_info si = std::filesystem::space(m_root);

			available = (size_t)si.available;
		}
		catch (std::exception& e)
		{
			JELLY_FATAL_ERROR("DefaultHost: Failed to determine available disk space for: %s (%s)", m_root.c_str(), e.what());
		}

		return available;
	}

	void		
	DefaultHost::EnumerateFiles(
		uint32_t					aNodeId,
		std::vector<uint32_t>&		aOutWriteAheadLogIds,
		std::vector<uint32_t>&		aOutStoreIds) 
	{
		try
		{
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
			{
				PathUtils::FileType fileType;
				uint32_t id;
				uint32_t nodeId;
				if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), m_filePrefix.c_str(), fileType, nodeId, id))
				{
					if(nodeId == aNodeId)
					{
						switch(fileType)
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
		catch (std::exception& e)
		{
			JELLY_FATAL_ERROR("DefaultHost: Failed to enumerate files for node id: %u (%s)", aNodeId, e.what());
		}
	}

	void		
	DefaultHost::GetStoreInfo(
		uint32_t					aNodeId,
		std::vector<StoreInfo>&		aOut) 
	{
		try
		{
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
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
		catch (std::exception& e)
		{
			JELLY_FATAL_ERROR("DefaultHost: Failed to get store info fornode id: %u (%s)", aNodeId, e.what());
		}
	}

	IFileStreamReader*
	DefaultHost::ReadWALStream(
		uint32_t					aNodeId,
		uint32_t					aId,
		bool						aUseStreamingCompression,
		FileStatsContext*			aFileStatsContext)
	{
		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider && aUseStreamingCompression ? m_compressionProvider->CreateStreamDecompressor() : NULL,
			aFileStatsContext));

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
		std::unique_ptr<WALWriter> f(new WALWriter(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider && aUseStreamingCompression ? m_compressionProvider->CreateStreamCompressor() : NULL,
			aFileStatsContext));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}
	
	void		
	DefaultHost::DeleteWAL(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		try
		{
			std::filesystem::remove(PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str());
		}
		catch (std::exception& e)
		{
			JELLY_FATAL_ERROR("DefaultHost: Failed to delete WAL (%u) for node id: %u (%s)", aId, aNodeId, e.what());
		}
	}
	
	IFileStreamReader*
	DefaultHost::ReadStoreStream(
		uint32_t					aNodeId,
		uint32_t					aId,
		FileStatsContext*			aFileStatsContext) 
	{
		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId).c_str(),
			NULL,
			aFileStatsContext));

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
		std::unique_ptr<StoreWriter> f(new StoreWriter(
			PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId).c_str(), aFileStatsContext));

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
		return new File(NULL, StringUtils::Format("%s/%snode-%u.lck", m_root.c_str(), m_filePrefix.c_str(), aNodeId).c_str(), File::MODE_MUTEX);
	}

}
