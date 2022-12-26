#include <assert.h>

#include <filesystem>

#include <jelly/Compression.h>
#include <jelly/Host.h>
#include <jelly/ZstdCompression.h>

#include "FileStreamReader.h"
#include "PathUtils.h"
#include "StoreBlobReader.h"
#include "StoreWriter.h"
#include "WALWriter.h"

namespace jelly
{

	Host::Host(
		const char*					aRoot,
		CompressionMode				aCompressionMode)
		: m_root(aRoot)
	{
		m_storeManager = std::make_unique<StoreManager>(aRoot);

		switch(aCompressionMode)
		{
		case COMPRESSION_MODE_ZSTD:		
			#if defined(JELLY_ZSTD)
				m_compressionProvider = std::make_unique<ZstdCompression>(); 
			#else
				assert(false);
			#endif
			break;

		default:
			break;
		}
	}
	
	Host::~Host()
	{

	}

	void		
	Host::DeleteAllFiles(
		uint32_t					aNodeId)
	{			
		m_storeManager->CloseAll();

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
		{
			PathUtils::FileType fileType;
			uint32_t id;
			uint32_t nodeId;
			if(entry.is_regular_file() && PathUtils::ParsePath(entry.path(), fileType, nodeId, id))
			{
				if(aNodeId == UINT32_MAX || nodeId == aNodeId)
				{
					std::string p = entry.path().u8string();

					std::filesystem::remove(p);
				}
			}
		}
	}

	//---------------------------------------------------------------

	Compression::IProvider* 
	Host::GetCompressionProvider() 
	{
		return m_compressionProvider.get();
	}

	uint64_t	
	Host::GetTimeStamp() 
	{
		return (uint64_t)time(NULL);
	}

	void		
	Host::EnumerateFiles(
		uint32_t					aNodeId,
		std::vector<uint32_t>&		aOutWriteAheadLogIds,
		std::vector<uint32_t>&		aOutStoreIds) 
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
		{
			PathUtils::FileType fileType;
			uint32_t id;
			uint32_t nodeId;
			if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), fileType, nodeId, id))
			{
				if(nodeId == aNodeId)
				{
					switch(fileType)
					{
					case PathUtils::FILE_TYPE_STORE:	aOutStoreIds.push_back(id); break;
					case PathUtils::FILE_TYPE_WAL:		aOutWriteAheadLogIds.push_back(id); break;
					}
				}
			}
		}

		std::sort(aOutStoreIds.begin(), aOutStoreIds.end());
		std::sort(aOutWriteAheadLogIds.begin(), aOutWriteAheadLogIds.end());
	}

	void		
	Host::GetStoreInfo(
		uint32_t					aNodeId,
		std::vector<StoreInfo>&		aOut) 
	{
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_root))
		{
			PathUtils::FileType fileType;
			uint32_t nodeId;
			uint32_t id;
			if (entry.is_regular_file() && PathUtils::ParsePath(entry.path(), fileType, nodeId, id))
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
	Host::ReadWALStream(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider ? m_compressionProvider->CreateStreamDecompressor() : NULL));

		if(!f->IsValid())
			return NULL;

		return f.release();
	}
	
	IWALWriter*
	Host::CreateWAL(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		std::unique_ptr<WALWriter> f(new WALWriter(
			PathUtils::MakePath(m_root.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str(),
			m_compressionProvider ? m_compressionProvider->CreateStreamCompressor() : NULL));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}
	
	void		
	Host::DeleteWAL(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		std::filesystem::remove(PathUtils::MakePath(m_root.c_str(), PathUtils::FILE_TYPE_WAL, aNodeId, aId).c_str());
	}
	
	IFileStreamReader*
	Host::ReadStoreStream(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		// If blob reader is open, close it, otherwise we can't stream it
		CloseStoreBlobReader(aNodeId, aId);

		std::unique_ptr<FileStreamReader> f(new FileStreamReader(
			PathUtils::MakePath(m_root.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId).c_str(),
			NULL));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}

	IStoreBlobReader* 
	Host::GetStoreBlobReader(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		return m_storeManager->GetStoreBlobReader(aNodeId, aId);
	}

	void					
	Host::CloseStoreBlobReader(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		IStoreBlobReader* storeBlobReader = m_storeManager->GetStoreBlobReaderIfExists(aNodeId, aId);

		if(storeBlobReader != NULL)
			storeBlobReader->Close();
	}
	
	IStoreWriter*
	Host::CreateStore(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		std::unique_ptr<StoreWriter> f(new StoreWriter(
			PathUtils::MakePath(m_root.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aId).c_str()));

		if (!f->IsValid())
			return NULL;

		return f.release();
	}

	void		
	Host::DeleteStore(
		uint32_t					aNodeId,
		uint32_t					aId) 
	{
		m_storeManager->DeleteStore(aNodeId, aId);
	}

}