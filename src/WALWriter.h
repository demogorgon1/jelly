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
						CompletionEvent*				aCompletionEvent) override;
		void		Flush() override;

	private:

		std::vector<std::pair<const IItem*, CompletionEvent*>>	m_pendingItemWrites;
		File													m_file;
		std::unique_ptr<Compression::IStreamCompressor>			m_compressor;
	};		

}