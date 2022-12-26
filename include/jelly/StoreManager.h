#pragma once

#include <mutex>
#include <unordered_map>

namespace jelly
{
	
	class IStoreBlobReader;

	class StoreManager
	{
	public:
							StoreManager(
								const char*		aRoot);
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

		struct Store;
		typedef std::unordered_map<uint64_t, Store*> Map;
		std::mutex	m_mapLock;
		Map			m_map;
	};

}