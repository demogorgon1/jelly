#pragma once

#include <jelly/File.h>
#include <jelly/IFileStreamReader.h>

namespace jelly
{

	namespace Compression
	{
		class IStreamDecompressor;
	}

	struct FileHeader;

	// DefaultHost implementation of IFileStreamReader
	class FileStreamReader
		: public IFileStreamReader
	{
	public:
					FileStreamReader(
						const char*							aPath,
						Compression::IStreamDecompressor*	aDecompressor,
						FileStatsContext*					aFileStatsContext,
						const FileHeader&					aFileHeader);
		virtual		~FileStreamReader();

		bool		IsValid() const;

		// IFileStreamReader implementation
		bool		IsEnd() const override;
		size_t		Read(
						void*								aBuffer,
						size_t								aBufferSize) override;
		size_t		GetTotalBytesRead() const override;

	private:

		File												m_file;
		size_t												m_offset;
		std::unique_ptr<Compression::IStreamDecompressor>	m_decompressor;
			
		struct Buffer;
		Buffer*												m_head;
		Buffer*												m_tail;
	};

}