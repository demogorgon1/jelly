#pragma once

namespace jelly
{
	
	class IStoreBlobReader;

	class StoreManager
	{
	public:
							StoreManager(
								const char*		aRoot,
								const char*		aFilePrefix);
							~StoreManager();

		IStoreBlobReader*	GetStoreBlobReader(
								uint32_t		aNodeId,
								uint32_t		aStoreId);
		IStoreBlobReader*	GetStoreBlobReaderIfExists(
								uint32_t		aNodeId,
								uint32_t		aStoreId);
		void				DeleteStore(
								uint32_t		aNodeId,
								uint32_t		aStoreId);
		void				CloseAll();

	private:
			
		std::string	m_root;
		std::string m_filePrefix;

		struct Store;
		typedef std::unordered_map<uint64_t, Store*> Map;
		std::mutex	m_mapLock;
		Map			m_map;
	};

}