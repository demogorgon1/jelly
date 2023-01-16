#include <jelly/Base.h>

#include <jelly/BlobBuffer.h>
#include <jelly/BufferReader.h>
#include <jelly/ErrorUtils.h>

#include "StoreBlobReader.h"

namespace jelly
{

	StoreBlobReader::StoreBlobReader(
		const char*			aPath)
		: m_path(aPath)
	{
		m_file = std::make_unique<File>(m_path.c_str(), File::MODE_READ_RANDOM);
	}

	StoreBlobReader::~StoreBlobReader()
	{
	}

	bool	
	StoreBlobReader::IsValid() const
	{
		return m_file && m_file->IsValid();
	}

	//--------------------------------------------------------------------------
		
	void		
	StoreBlobReader::ReadItemBlob(
		size_t				aOffset,
		IItem*				aItem)
	{
		// FIXME: this is obviously ridiculous... don't open file for every read
		if(!m_file)
			m_file = std::make_unique<File>(m_path.c_str(), File::MODE_READ_RANDOM);

		JELLY_ASSERT(m_file);

		std::unique_ptr<BlobBuffer> buffer = std::make_unique<BlobBuffer>();
		buffer->SetSize(aItem->GetStoredBlobSize());

		m_file->ReadAtOffset(aOffset, buffer->GetPointer(), buffer->GetSize());

		aItem->UpdateBlobBuffer(buffer);
	}

	void		
	StoreBlobReader::Close() 
	{
		m_file = NULL;
	}

}