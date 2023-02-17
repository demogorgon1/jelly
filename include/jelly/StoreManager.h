#pragma once

#include "FileHeader.h"

namespace jelly
{
	
	struct FileStatsContext;

	class IStoreBlobReader;

	// DefaultHost store manager
	class StoreManager
	{
	public:
							StoreManager(
								const char*			aRoot,
								const char*			aFilePrefix,
								const FileHeader&	aFileHeader);
							~StoreManager();

		IStoreBlobReader*	GetStoreBlobReader(
								uint32_t			aNodeId,
								uint32_t			aStoreId,
								FileStatsContext*	aFileStatsContext);
		IStoreBlobReader*	GetStoreBlobReaderIfExists(
								uint32_t			aNodeId,
								uint32_t			aStoreId);
		void				DeleteStore(
								uint32_t			aNodeId,
								uint32_t			aStoreId);
		void				CloseAll();

	private:
			
		std::string	m_root;
		std::string m_filePrefix;
		FileHeader	m_fileHeader;

		struct Store;
		typedef std::unordered_map<uint64_t, Store*> Map;
		std::mutex	m_mapLock;
		Map			m_map;
	};

}