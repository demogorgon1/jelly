#pragma once

#include <jelly/File.h>
#include <jelly/FileHeader.h>
#include <jelly/ItemBase.h>
#include <jelly/IStoreBlobReader.h>

namespace jelly
{

	// DefaultHost implementation of IStoreBlobReader
	class StoreBlobReader
		: public IStoreBlobReader
	{
	public:
					StoreBlobReader(
						const char*			aPath,
						FileStatsContext*	aFileStatsContext,
						const FileHeader&	aFileHeader) noexcept;
		virtual		~StoreBlobReader();

		bool		IsValid() const noexcept;

		// IStoreBlobReader
		void		ReadItemBlob(
						size_t				aOffset, 
						ItemBase*			aItem) override;
		void		Close() override;

	private:

		std::string					m_path;
		std::unique_ptr<File>		m_file;	
		FileStatsContext*			m_fileStatsContext;
		FileHeader					m_fileHeader;
	};

}