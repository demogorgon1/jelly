#pragma once

namespace jelly
{

	namespace Test
	{

		class MemoryHost
			: public IHost
		{
		public:
									MemoryHost();
			virtual					~MemoryHost();

			void					DeleteAllFiles(
										uint32_t					aNodeId);

			// IHost implementation
			IStats*					GetStats() override;
			IConfigSource*			GetConfigSource() override;
			Compression::IProvider* GetCompressionProvider() override;
			uint64_t				GetTimeStamp() override;
			size_t					GetAvailableDiskSpace() override;
			void					EnumerateFiles(
										uint32_t					aNodeId,
										std::vector<uint32_t>& aOutWriteAheadLogIds,
										std::vector<uint32_t>& aOutStoreIds) override;
			void					GetStoreInfo(
										uint32_t					aNodeId,
										std::vector<StoreInfo>&		aOut) override;
			IFileStreamReader*		ReadWALStream(
										uint32_t					aNodeId,
										uint32_t					aId,
										bool						aUseStreamingCompression,
										FileStatsContext*			aFileStatsContext) override;
			IWALWriter*				CreateWAL(
										uint32_t					aNodeId,
										uint32_t					aId,
										bool						aUseStreamingCompression,
										FileStatsContext*			aFileStatsContext) override;
			void					DeleteWAL(
										uint32_t					aNodeId,
										uint32_t					aId) override;
			IFileStreamReader*		ReadStoreStream(
										uint32_t					aNodeId,
										uint32_t					aId,
										FileStatsContext*			aFileStatsContext) override;
			IStoreBlobReader*		GetStoreBlobReader(
										uint32_t					aNodeId,
										uint32_t					aId,
										FileStatsContext*			aFileStatsContext) override;
			IStoreWriter*			CreateStore(
										uint32_t					aNodeId,
										uint32_t					aId,
										FileStatsContext*			aFileStatsContext) override;
			void					DeleteStore(
										uint32_t					aNodeId,
										uint32_t					aId) override;
			File*					CreateNodeLock(
										uint32_t					aNodeId) override;
			bool					GetLatestBackupInfo(
										uint32_t					aNodeId,
										const char*					aBackupPath,
										std::string&				aOutName,
										uint32_t&					aOutLatestStoreId) override;
			std::string				GetStorePath(
										uint32_t					aNodeId,
										uint32_t					aId) override;

			// Data access
			DefaultConfigSource*	GetDefaultConfigSource() { return &m_defaultConfigSource; }

		private:

			struct Store;
			struct WAL;

			typedef std::map<std::pair<uint32_t, uint32_t>, Store*> StoreMap;
			typedef std::map<std::pair<uint32_t, uint32_t>, WAL*> WALMap;

			StoreMap								m_storeMap;
			WALMap									m_walMap;
			std::atomic_uint64_t					m_timeStamp;

			DefaultConfigSource						m_defaultConfigSource;
		};

	}

}