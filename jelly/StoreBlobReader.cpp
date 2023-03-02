#include <jelly/Base.h>

#include <jelly/Buffer.h>
#include <jelly/BufferReader.h>
#include <jelly/ErrorUtils.h>

#include "StoreBlobReader.h"

namespace jelly
{

	StoreBlobReader::StoreBlobReader(
		const char*			aPath,
		FileStatsContext*	aFileStatsContext,
		const FileHeader&	aFileHeader) noexcept
		: m_path(aPath)
		, m_fileStatsContext(aFileStatsContext)
		, m_fileHeader(aFileHeader)
	{
		m_file = std::make_unique<File>(m_fileStatsContext, m_path.c_str(), File::MODE_READ_RANDOM, m_fileHeader);
	}

	StoreBlobReader::~StoreBlobReader()
	{
	}

	bool	
	StoreBlobReader::IsValid() const noexcept
	{
		return m_file && m_file->IsValid();
	}

	//--------------------------------------------------------------------------
		
	void		
	StoreBlobReader::ReadItemBlob(
		size_t				aOffset,
		ItemBase*			aItem)
	{
		if(!m_file)
			m_file = std::make_unique<File>(m_fileStatsContext, m_path.c_str(), File::MODE_READ_RANDOM, m_fileHeader);

		JELLY_ASSERT(m_file);

		std::unique_ptr<IBuffer> buffer = std::make_unique<Buffer<1>>();
		buffer->SetSize(aItem->GetStoredBlobSize());

		m_file->ReadAtOffset(aOffset, buffer->GetPointer(), buffer->GetSize());

		aItem->UpdateBlobBuffer(buffer);
	}

	void		
	StoreBlobReader::Close() 
	{
		m_file.reset();
	}

}