#pragma once

#include "IHost.h"
#include "IStats.h"
#include "StoreManager.h"

namespace jelly
{

	class DefaultHost
		: public IHost
	{
	public:
								DefaultHost(	
									const char*					aRoot,
									const char*					aFilePrefix,
									Compression::Id				aCompressionId,
									uint32_t					aBufferCompressionLevel = 0);
		virtual					~DefaultHost();

		void					DeleteAllFiles(
									uint32_t					aNodeId);

		// IHost implementation
		IStats*					GetStats() override;
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
									uint32_t					aId,
									bool						aUseStreamingCompression) override;
		IWALWriter*				CreateWAL(
									uint32_t					aNodeId,
									uint32_t					aId,
									bool						aUseStreamingCompression) override;
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
		std::string								m_filePrefix;
		std::unique_ptr<Compression::IProvider>	m_compressionProvider;
		std::unique_ptr<StoreManager>			m_storeManager;
		std::unique_ptr<IStats>					m_stats;
	};

}