#pragma once

#include <jelly/IFileStreamReader.h>

#include "File.h"

namespace jelly
{

	namespace Compression
	{
		class IStreamDecompressor;
	}

	class FileStreamReader
		: public IFileStreamReader
	{
	public:
					FileStreamReader(
						const char*							aPath,
						Compression::IStreamDecompressor*	aDecompressor);
		virtual		~FileStreamReader();

		bool		IsValid() const;

		// IFileStreamReader implementation
		bool		IsEnd() const override;
		size_t		GetReadOffset() const override;
		size_t		Read(
						void*								aBuffer,
						size_t								aBufferSize) override;

	private:

		File												m_file;
		size_t												m_offset;
		std::unique_ptr<Compression::IStreamDecompressor>	m_decompressor;
			
		struct Buffer;
		Buffer*												m_head;
		Buffer*												m_tail;
	};

}