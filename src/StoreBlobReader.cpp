#include <assert.h>
#include <stdint.h>

#include "BufferReader.h"
#include "StoreBlobReader.h"

namespace jelly
{

	namespace Impl
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
			if(!m_file)
				m_file = std::make_unique<File>(m_path.c_str(), File::MODE_READ_RANDOM);

			assert(m_file);

			size_t bufferSize = aItem->GetStoredBlobSize();
			uint8_t* buffer = new uint8_t[bufferSize];

			try
			{
				m_file->ReadAtOffset(aOffset, buffer, bufferSize);

				// FIXME: we should basically just feed the data directly into the decompressor

				BufferReader reader(buffer, bufferSize);

				if (!aItem->Read(&reader, NULL, IItem::READ_TYPE_BLOB_ONLY))
					assert(false);
			}
			catch(...)
			{
				assert(false);
			}

			delete [] buffer;
		}

		void		
		StoreBlobReader::Close() 
		{
			m_file = NULL;
		}

	}

}