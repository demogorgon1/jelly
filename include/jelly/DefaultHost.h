#pragma once

#include "IHost.h"
#include "IStats.h"
#include "StoreManager.h"

namespace jelly
{

	/**
	 * \brief Default implementation of the IHost interface, which nodes use to interact with the system.
	 * 
	 * \see IHost
	 */
	class DefaultHost
		: public IHost
	{
	public:
		/**
		 * \param aRoot				          Path to directory where database files should be stored.
		 * \param aFilePrefix		          String the will prefixed to database files. Useful if you're storing multiple databases in the same directory.
		 * \param aCompressionId	          Specifies compression algorithm.
		 * \param aBufferCompressionLevel	  Blob compression level in the range of 1 (lowest, fastest) to 9 (highest, slowest). A compression level of 1 is usually plenty when using ZSTD and is very fast.
		 * \param aExtraApplicationStats      Pointer to application-defined statistics.
		 * \param aExtraApplicationStatsCount Number of elements pointed to by aExtraApplicationStats.
		 */
								DefaultHost(	
									const char*					aRoot,
									const char*					aFilePrefix,
									Compression::Id				aCompressionId,
									uint32_t					aBufferCompressionLevel = 0,
									const Stat::Info*			aExtraApplicationStats = NULL,
									uint32_t					aExtraApplicationStatsCount = 0);
		virtual					~DefaultHost();

		//! Poll system stats.
		void					PollSystemStats();

		//! Deletes all database files associated with specified node id.
		void					DeleteAllFiles(
									uint32_t					aNodeId = UINT32_MAX);

		//----------------------------------------------------------------------------------------------
		// IHost implementation
		IStats*					GetStats() override;
		Compression::IProvider* GetCompressionProvider() override;
		uint64_t				GetTimeStamp() override;
		size_t					GetAvailableDiskSpace() override;
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

	private:
		
		std::string								m_root;
		std::string								m_filePrefix;
		std::unique_ptr<Compression::IProvider>	m_compressionProvider;
		std::unique_ptr<StoreManager>			m_storeManager;
		std::unique_ptr<IStats>					m_stats;
	};

}