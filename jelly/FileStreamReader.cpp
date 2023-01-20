#include <jelly/Base.h>

#include <jelly/Compression.h>
#include <jelly/ErrorUtils.h>

#include "FileStreamReader.h"

namespace jelly
{

	struct FileStreamReader::Buffer
	{
		static constexpr size_t SIZE = 16 * 1024;

		Buffer()
			: m_numBytes(0)
			, m_readOffset(0)
			, m_next(NULL)
		{

		}

		uint8_t		m_data[SIZE];
		size_t		m_readOffset;
		size_t		m_numBytes;
		Buffer*		m_next;
	};

	//-------------------------------------------------------------------------------------------

	FileStreamReader::FileStreamReader(
		const char*							aPath,
		Compression::IStreamDecompressor*	aDecompressor,
		FileStatsContext*					aFileStatsContext)
		: m_file(aFileStatsContext, aPath, File::MODE_READ_STREAM)
		, m_offset(0)
		, m_head(NULL)
		, m_tail(NULL)
		, m_decompressor(aDecompressor)
	{
		if(m_decompressor)
		{
			m_decompressor->SetOutputCallback([&](
				const void*					aBuffer,
				size_t						aBufferSize)
			{
				uint8_t* p = (uint8_t*)aBuffer;
				size_t remaining = aBufferSize;

				if(m_tail != NULL && m_tail->m_numBytes < Buffer::SIZE)
				{
					// We can copy at least some of the new data into the tail buffer, maybe we can avoid an allocation
					size_t toCopy = std::min<size_t>(Buffer::SIZE - m_tail->m_numBytes, remaining);
					memcpy(m_tail->m_data + m_tail->m_numBytes, p, toCopy);
					remaining -= toCopy;
					p += toCopy;
					m_tail->m_numBytes += toCopy;
				}

				while(remaining > 0)
				{
					std::unique_ptr<Buffer> buffer(new Buffer());
					buffer->m_numBytes = std::min<size_t>(Buffer::SIZE, remaining);
					memcpy(buffer->m_data, p, buffer->m_numBytes);
					p += buffer->m_numBytes;
					remaining -= buffer->m_numBytes;

					if(m_tail != NULL)
						m_tail->m_next = buffer.get();
					else
						m_head = buffer.get();
					m_tail = buffer.release();
				}
			});
		}
	}

	FileStreamReader::~FileStreamReader()
	{
		Buffer* buffer = m_head;
		while(buffer != NULL)
		{
			Buffer* next = buffer->m_next;
			delete buffer;
			buffer = next;
		}
	}

	bool
	FileStreamReader::IsValid() const
	{
		return m_file.IsValid();
	}

	//-------------------------------------------------------------------------------------------

	bool	
	FileStreamReader::IsEnd() const 
	{
		if(m_decompressor)
		{
			if(m_offset < m_file.GetSize())
				return false;

			return m_head == NULL;
		}
			
		return m_file.GetSize() == m_offset;
	}

	size_t		
	FileStreamReader::GetReadOffset() const 
	{
		return m_offset;
	}

	//-------------------------------------------------------------------------------------------

	size_t
	FileStreamReader::Read(
		void*				aBuffer,
		size_t				aBufferSize) 
	{
		if(m_decompressor)
		{
			size_t bytes = 0;

			uint8_t* p = (uint8_t*)aBuffer;
			size_t remaining = aBufferSize;

			while(remaining  > 0)
			{
				if(m_head == NULL)
				{
					uint8_t compressed[32768];
					size_t compressedSize = m_file.Read(compressed, sizeof(compressed));
					if(compressedSize == 0)
					{
						break;
					}

					m_offset += compressedSize;

					m_decompressor->FeedData(compressed, compressedSize);

					if (m_head == NULL)
						break;
				}

				JELLY_ASSERT(m_head->m_numBytes > m_head->m_readOffset);

				size_t toCopy = std::min<size_t>(m_head->m_numBytes - m_head->m_readOffset, remaining);
				memcpy(p, &m_head->m_data[m_head->m_readOffset], toCopy);
				m_head->m_readOffset += toCopy;
				p += toCopy;
				remaining -= toCopy;
				bytes += toCopy;

				if(m_head->m_readOffset == m_head->m_numBytes)
				{
					Buffer* next = m_head->m_next;
					delete m_head;
					m_head = next;
					if(m_head == NULL)
						m_tail = NULL;
				}
			}

			return bytes;
		}
		else
		{
			size_t bytes = m_file.Read(aBuffer, aBufferSize);
			m_offset += bytes;
			return bytes;
		}
	}

}