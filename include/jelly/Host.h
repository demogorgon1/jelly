#pragma once

#include <string>

#include "IHost.h"
#include "StoreManager.h"

namespace jelly
{

	class Host
		: public IHost
	{
	public:
		enum CompressionMode
		{
			COMPRESSION_MODE_NONE,
			COMPRESSION_MODE_ZSTD
		};
								Host(	
									const char*					aRoot,
									CompressionMode				aCompressionMode);
		virtual					~Host();

		void					DeleteAllFiles(
									uint32_t					aNodeId);

		// IHost implementation
		Compression::IProvider* GetCompressionProvider() override;
		uint64_t				GetTimeStamp() override;
		void					EnumerateFiles(
									uint32_t					aNodeId,
									std::vector<uint32_t>&		aOutWriteAheadLogIds,
									std::vector<uint32_t>&		aOutStoreIds) override;
		void					GetStoreInfo(
									uint32_t					aNodeId,
									std::vector<StoreInfo>&		aOut) override;
		IFileStreamReader*		ReadWALStream(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		IWALWriter*				CreateWAL(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		void					DeleteWAL(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		IFileStreamReader*		ReadStoreStream(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		IStoreBlobReader*		GetStoreBlobReader(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		void					CloseStoreBlobReader(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		IStoreWriter*			CreateStore(
									uint32_t					aNodeId,
									uint32_t					aId) override;
		void					DeleteStore(
									uint32_t					aNodeId,
									uint32_t					aId) override;

	private:
		
		std::string								m_root;
		std::unique_ptr<Compression::IProvider>	m_compressionProvider;
		std::unique_ptr<StoreManager>			m_storeManager;
	};

}