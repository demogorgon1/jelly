#include <jelly/Base.h>

#include <jelly/StoreManager.h>

#include "PathUtils.h"
#include "StoreBlobReader.h"

namespace jelly
{

	struct StoreManager::Store
	{
		Store(
			const char*			aRoot,
			const char*			aFilePrefix,
			uint32_t			aNodeId,
			uint32_t			aStoreId,
			FileStatsContext*	aFileStatsContext,
			const FileHeader&	aFileHeader)
			: m_nodeId(aNodeId)
			, m_storeId(aStoreId)
		{
			m_blobReader = std::make_unique<StoreBlobReader>(
				PathUtils::MakePath(aRoot, aFilePrefix, PathUtils::FILE_TYPE_STORE, m_nodeId, m_storeId).c_str(), aFileStatsContext, aFileHeader);
		}

		// Public data
		uint32_t							m_nodeId;
		uint32_t							m_storeId;
		std::unique_ptr<StoreBlobReader>	m_blobReader;
	};

	//------------------------------------------------------------------------------

	StoreManager::StoreManager(
		const char*			aRoot,
		const char*			aFilePrefix,
		const FileHeader&	aFileHeader)
		: m_root(aRoot)
		, m_filePrefix(aFilePrefix)
		, m_fileHeader(aFileHeader)
	{

	}
		
	StoreManager::~StoreManager()
	{
		CloseAll();
	}

	IStoreBlobReader* 
	StoreManager::GetStoreBlobReader(
		uint32_t			aNodeId,
		uint32_t			aStoreId,
		FileStatsContext*	aFileStatsContext)
	{
		StoreBlobReader* blobReader = NULL;

		uint64_t key = (((uint64_t)aNodeId) << 32) | (uint64_t)aStoreId;

		// Note that we only need to protect the map itself with a mutex, as individual
		// store blob readers are tied to specific nodes that are bound to a single thread at a time

		{
			std::lock_guard lock(m_mapLock);

			Map::iterator i = m_map.find(key);
			if (i != m_map.end())
				return i->second->m_blobReader.get();
		}

		std::unique_ptr<Store> store(new Store(m_root.c_str(), m_filePrefix.c_str(), aNodeId, aStoreId, aFileStatsContext, m_fileHeader));

		if(!store->m_blobReader->IsValid())
			return NULL;

		blobReader = store->m_blobReader.get();

		{
			std::lock_guard lock(m_mapLock);

			m_map[key] = store.release();
		}

		return blobReader;
	}

	IStoreBlobReader* 
	StoreManager::GetStoreBlobReaderIfExists(
		uint32_t		aNodeId,
		uint32_t		aStoreId)
	{
		uint64_t key = (((uint64_t)aNodeId) << 32) | (uint64_t)aStoreId;

		{
			std::lock_guard lock(m_mapLock);

			Map::iterator i = m_map.find(key);
			if (i == m_map.end())
				return NULL;

			return i->second->m_blobReader.get();
		}
	}

	void				
	StoreManager::DeleteStore(
		uint32_t		aNodeId,
		uint32_t		aStoreId)
	{
		uint64_t key = (((uint64_t)aNodeId) << 32) | (uint64_t)aStoreId;

		std::unique_ptr<Store> store;

		{
			std::lock_guard lock(m_mapLock);

			Map::iterator i = m_map.find(key);
			if (i != m_map.end())
			{
				store = std::unique_ptr<Store>(i->second);

				m_map.erase(key);
			}
		}

		if(store)
			store->m_blobReader->Close();

		std::filesystem::remove(PathUtils::MakePath(m_root.c_str(), m_filePrefix.c_str(), PathUtils::FILE_TYPE_STORE, aNodeId, aStoreId).c_str());
	}

	void				
	StoreManager::CloseAll()
	{
		for(Map::iterator i = m_map.begin(); i != m_map.end(); i++)
			delete i->second;

		m_map.clear();
	}

}