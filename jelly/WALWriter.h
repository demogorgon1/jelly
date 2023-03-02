#pragma once

#include <jelly/Compression.h>
#include <jelly/File.h>
#include <jelly/IWALWriter.h>

namespace jelly
{

	struct FileHeader;

	class IStats;

	// DefaultHost implementation of IWALWriter
	class WALWriter
		: public IWALWriter
	{
	public:
					WALWriter(
						const char*						aPath,
						const Compression::IProvider*	aCompression,
						FileStatsContext*				aFileStatsContext,
						const FileHeader&				aFileHeader);
		virtual		~WALWriter();

		bool		IsValid() const;

		// IWALWriter implementation
		size_t		GetSize() const override;
		void		WriteItem(
						const ItemBase*					aItem,
						CompletionEvent*				aCompletionEvent,
						RequestResult*					aResult,
						Exception::Code*				aException) override;
		size_t		Flush(
						ReplicationNetwork*				aReplicationNetwork) override;
		void		Cancel() override;
		size_t		GetPendingWriteCount() const override;
		bool		HadFailure() const override;

	private:

		struct PendingItemWrite
		{
			const ItemBase*										m_item;
			CompletionEvent*									m_completionEvent;
			RequestResult*										m_result;
			Exception::Code*									m_exception;
		};

		std::vector<PendingItemWrite>							m_pendingItemWrites;
		File													m_file;
		std::unique_ptr<Compression::IStreamCompressor>			m_compressor;
		const Compression::IProvider*							m_compression;
		bool													m_hadFailure;
	};		

}