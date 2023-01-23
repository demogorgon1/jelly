#pragma once

#include <jelly/File.h>
#include <jelly/IWALWriter.h>

namespace jelly
{
	
	namespace Compression
	{
		class IStreamCompressor;
	}

	class IStats;

	class WALWriter
		: public IWALWriter
	{
	public:
					WALWriter(
						const char*						aPath,
						Compression::IStreamCompressor*	aCompressor,
						FileStatsContext*				aFileStatsContext);
		virtual		~WALWriter();

		bool		IsValid() const;

		// IWALWriter implementation
		size_t		GetSize() const override;
		void		WriteItem(
						const IItem*					aItem,
						CompletionEvent*				aCompletionEvent,
						Result*							aResult) override;
		size_t		Flush() override;
		void		Cancel() override;
		size_t		GetPendingWriteCount() const override;

	private:

		struct PendingItemWrite
		{
			const IItem*										m_item;
			CompletionEvent*									m_completionEvent;
			Result*												m_result;
		};

		std::vector<PendingItemWrite>							m_pendingItemWrites;
		File													m_file;
		std::unique_ptr<Compression::IStreamCompressor>			m_compressor;
	};		

}