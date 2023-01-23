#pragma once

namespace jelly
{

	namespace Compression
	{
		class IProvider;
	}

	struct FileStatsContext;

	class IFileStreamReader;
	class IStats;
	class IStoreBlobReader;
	class IStoreWriter;
	class IWALWriter;

	// Interface for host implementation. 
	class IHost
	{
	public:
		virtual							~IHost() {}

		struct StoreInfo
		{
			uint32_t		m_id;
			size_t			m_size;

			bool operator<(const StoreInfo& aOther) const { return m_id < aOther.m_id; }
		};

		// Virtual interface
		virtual IStats*					GetStats() = 0;
		virtual Compression::IProvider*	GetCompressionProvider() = 0;
		virtual uint64_t				GetTimeStamp() = 0;
		virtual void					EnumerateFiles(
											uint32_t				aNodeId,
											std::vector<uint32_t>&	aOutWriteAheadLogIds,
											std::vector<uint32_t>&	aOutStoreIds) = 0;
		virtual void					GetStoreInfo(
											uint32_t				aNodeId,
											std::vector<StoreInfo>&	aOut) = 0;
		virtual IFileStreamReader*		ReadWALStream(
											uint32_t				aNodeId,
											uint32_t				aId,
											bool					aUseStreamingCompression,
											FileStatsContext*		aFileStatsContext) = 0;
		virtual IWALWriter*				CreateWAL(
											uint32_t				aNodeId,
											uint32_t				aId,
											bool					aUseStreamingCompression,
											FileStatsContext*		aFileStatsContext) = 0;
		virtual void					DeleteWAL(
											uint32_t				aNodeId,
											uint32_t				aId) = 0;
		virtual IFileStreamReader*		ReadStoreStream(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;
		virtual IStoreBlobReader*		GetStoreBlobReader(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;
		virtual IStoreWriter*			CreateStore(
											uint32_t				aNodeId,
											uint32_t				aId,
											FileStatsContext*		aFileStatsContext) = 0;
		virtual void					DeleteStore(
											uint32_t				aNodeId,
											uint32_t				aId) = 0;

	};

}