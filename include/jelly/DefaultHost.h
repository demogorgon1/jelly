#pragma once

#include "File.h"
#include "IHost.h"
#include "IStats.h"
#include "StoreManager.h"

namespace jelly
{

	class DefaultConfigSource;

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
		 * \param aConfigSource				  Configuration source interface.
		 * \param aExtraApplicationStats      Pointer to application-defined statistics.
		 * \param aExtraApplicationStatsCount Number of elements pointed to by aExtraApplicationStats.
		 */
								DefaultHost(	
									const char*					aRoot = ".",
									const char*					aFilePrefix = "",
									const IConfigSource*		aConfigSource = NULL,
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
		const IConfigSource*	GetConfigSource() override;
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
		DefaultConfigSource*	GetDefaultConfigSource() { JELLY_ASSERT(m_defaultConfigSource); return m_defaultConfigSource.get(); }

	private:
		
		std::string								m_root;
		std::string								m_filePrefix;
		const IConfigSource*					m_configSource;
		std::unique_ptr<Compression::IProvider>	m_compressionProvider;
		std::unique_ptr<StoreManager>			m_storeManager;
		std::unique_ptr<IStats>					m_stats;
		std::unique_ptr<DefaultConfigSource>	m_defaultConfigSource;
		std::unique_ptr<ConfigProxy>			m_config;
		std::unique_ptr<File>					m_fileLock;
	};

}
