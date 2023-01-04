#pragma once

#include <vector>

#include <jelly/IWALWriter.h>

#include "File.h"

namespace jelly
{
	
	namespace Compression
	{
		class IStreamCompressor;
	}

	class WALWriter
		: public IWALWriter
	{
	public:
					WALWriter(
						const char*						aPath,
						Compression::IStreamCompressor*	aCompressor);
		virtual		~WALWriter();

		bool		IsValid() const;

		// IWALWriter implementation
		size_t		GetSize() override;
		void		WriteItem(
						const IItem*					aItem,
						CompletionEvent*				aCompletionEvent,
						Result*							aResult) override;
		void		Flush() override;
		void		Cancel() override;

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