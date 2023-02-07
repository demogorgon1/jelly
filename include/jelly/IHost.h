#pragma once

namespace jelly
{

	namespace Compression
	{
		class IProvider;
	}

	struct FileStatsContext;

	class IConfigSource;
	class IFileStreamReader;
	class IStats;
	class IStoreBlobReader;
	class IStoreWriter;
	class IWALWriter;

	/**
	 * \brief Abstract interface for nodes to interfact with the system with. 
	 * 
	   DefaultHost provides a default implementation
	 * of this interface. You don't need to have a host object per node: multiple nodes can share the same one if you have
	 * multiple nodes hosted in the same process. The idea is that you'll probably want to have one host object per process.
	 * 
	 * \see DefaultHost
	 */
	class IHost
	{
	public:
		virtual							~IHost() {}

		struct StoreInfo
		{
			uint32_t		m_id = 0;
			size_t			m_size = 0;

			bool operator<(const StoreInfo& aOther) const { return m_id < aOther.m_id; }
		};

		//---------------------------------------------------------------------------
		// Virtual interface

		//! Get statistics object for querying or emitting database metrics.
		virtual IStats*					GetStats() = 0;

		//! Get config source.
		virtual IConfigSource*			GetConfigSource() = 0;

		//! Get compression provider for managing compression.
		virtual Compression::IProvider*	GetCompressionProvider() = 0;

		//! Get current system time stamp. 
		virtual uint64_t				GetTimeStamp() = 0;

		//! Get available disk space (in root directory) in bytes.
		virtual size_t					GetAvailableDiskSpace() = 0;

		//! Find database files for the specified node id. Returns two arrays: one with WAL ids and one with store ids.
		virtual void					EnumerateFiles(
											uint32_t				aNodeId,
											std::vector<uint32_t>&	aOutWriteAheadLogIds,
											std::vector<uint32_t>&	aOutStoreIds) = 0;

		//! Find stores (and their sizes) for the specified node id. 
		virtual void					GetStoreInfo(
											uint32_t				aNodeId,
											std::vector<StoreInfo>&	aOut) = 0;

		//! Open an existing WAL for reading. 
		virtual IFileStreamReader*		ReadWALStream(
											uint32_t				aNodeId,
											uint32_t				aId,
											bool					aUseStreamingCompression,
											FileStatsContext*		aFileStatsContext) = 0;

		//! Create a new WAL.
		virtual IWALWriter*				CreateWAL(
											uint32_t				aNodeId,
											uint32_t				aId,
											bool					aUseStreamingCompression,
											FileStatsContext*		aFileStatsContext) = 0;

		//! Delete an existing WAL.
		virtual void					DeleteWAL(
											uint32_t				aNodeId,
											uint32_t				aId) = 0;

		//! Open an existing store for (streamed) reading.
		virtual IFileStreamReader*		ReadStoreStream(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;

		//! Open an existing store for (random access) reading.
		virtual IStoreBlobReader*		GetStoreBlobReader(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;

		//! Create a new store.
		virtual IStoreWriter*			CreateStore(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;
		//! Delete an existing store.
		virtual void					DeleteStore(
											uint32_t				aNodeId,
											uint32_t				aId) = 0;

	};

}