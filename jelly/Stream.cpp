#include <jelly/Base.h>

#include <jelly/Stream.h>

namespace jelly
{


	namespace Stream
	{

		namespace
		{

			void
			_WriteToStream(
				Buffer*&								aHead,
				Buffer*&								aTail,
				const void*								aBuffer,
				size_t									aBufferSize)
			{
				size_t remaining = aBufferSize;
				const uint8_t* p = (const uint8_t*)aBuffer;

				while (remaining > 0)
				{
					size_t spaceLeftInBuffer = aTail != NULL ? Buffer::MAX_SIZE - aTail->m_size : 0;

					if (spaceLeftInBuffer == 0)
					{
						Buffer* newBuffer = new Buffer();

						if (aTail != NULL)
							aTail->m_next = newBuffer;
						else
							aHead = newBuffer;

						aTail = newBuffer;

						spaceLeftInBuffer = Buffer::MAX_SIZE;
					}

					JELLY_ASSERT(aTail != NULL);
					JELLY_ASSERT(spaceLeftInBuffer > 0);
					JELLY_ASSERT(aTail->m_size < Buffer::MAX_SIZE);

					size_t toCopy = std::min<size_t>(remaining, spaceLeftInBuffer);
					memcpy(aTail->m_data + aTail->m_size, p, toCopy);

					p += toCopy;
					aTail->m_size += toCopy;
					remaining -= toCopy;
				}
			}

		}

		//---------------------------------------------------------------------------------

		Writer::Writer(
			const Compression::IProvider*				aCompression)
			: m_head(NULL)
			, m_tail(NULL)
			, m_totalBytesWritten(0)
		{
			if(aCompression != NULL)
			{
				m_compressor.reset(aCompression->CreateStreamCompressor());

				m_compressor->SetOutputCallback([&](
					const void*								aBuffer,
					size_t									aBufferSize)
				{
					_WriteToStream(m_head, m_tail, aBuffer, aBufferSize);
				});
			}
		}

		Writer::~Writer()
		{
			if(m_head != NULL)
				delete m_head;
		}

		Buffer*			
		Writer::DetachBuffers()
		{
			if (m_compressor)
				m_compressor->Flush();

			Buffer* head = m_head;
			m_head = NULL;
			m_tail = NULL;
			return head;
		}

		void			
		Writer::Append(
			Writer&										aOther)
		{
			if(aOther.m_compressor)
				aOther.m_compressor->Flush();

			if(aOther.m_head == NULL)
				return;

			if (m_head != NULL)
				m_head->m_next = aOther.m_head;
			else
				m_head = aOther.m_head;

			m_tail = aOther.m_tail;

			aOther.m_head = NULL;
			aOther.m_tail = NULL;
		}

		size_t			
		Writer::Write(
			const void*									aBuffer,
			size_t										aBufferSize) 
		{
			if(m_compressor)
			{
				size_t bytes = m_compressor->Write(aBuffer, aBufferSize);
				JELLY_ASSERT(bytes == aBufferSize);
			}
			else
			{
				_WriteToStream(m_head, m_tail, aBuffer, aBufferSize);				
			}

			m_totalBytesWritten += aBufferSize;

			return aBufferSize;
		}
	
		size_t			
		Writer::GetTotalBytesWritten() const 
		{
			return m_totalBytesWritten;
		}

		//---------------------------------------------------------------------------------

		Reader::Reader(
			const Compression::IProvider*				aCompression,
			const Buffer*								aHead)
			: m_decompressedHead(NULL)
			, m_decompressedTail(NULL)
			, m_totalBytesRead(0)
			, m_headReadOffset(0)
		{
			if(aCompression != NULL)
			{
				m_decompressor.reset(aCompression->CreateStreamDecompressor());

				m_decompressor->SetOutputCallback([&](
					const void*								aBuffer,
					size_t									aBufferSize)
				{
					_WriteToStream(m_decompressedHead, m_decompressedTail, aBuffer, aBufferSize);
				});
			}

			if(aHead != NULL)
				Append(aHead);
		}
		
		Reader::~Reader()
		{
			if (m_decompressedHead != NULL)
				delete m_decompressedHead;
		}

		void			
		Reader::Append(
			const Buffer*								aHead)
		{
			JELLY_ASSERT(aHead != NULL);

			for (const Buffer* buffer = aHead; buffer != NULL; buffer = buffer->m_next)
			{
				if (m_decompressor)
					m_decompressor->FeedData(buffer->m_data, buffer->m_size);
				else
					m_rawBuffers.push_back(buffer);
			}
		}

		bool			
		Reader::IsEnd() const
		{
			if(m_decompressor)
				return m_decompressedHead == NULL;

			return m_rawBuffers.size() == 0;
		}

		size_t			
		Reader::Read(
			void*										aBuffer,
			size_t										aBufferSize) 
		{
			uint8_t* p = (uint8_t*)aBuffer;
			size_t remaining = aBufferSize;
			size_t bytesRead = 0;

			if(m_decompressor)
			{
				while(remaining > 0 && m_decompressedHead != NULL)
				{
					JELLY_ASSERT(m_headReadOffset < m_decompressedHead->m_size);
					size_t remainingInHeadBuffer = m_decompressedHead->m_size - m_headReadOffset;

					size_t toCopy = std::min<size_t>(remaining, remainingInHeadBuffer);

					memcpy(p, m_decompressedHead->m_data + m_headReadOffset, toCopy);

					p += toCopy;
					bytesRead += toCopy;
					remaining -= toCopy;
					m_headReadOffset += toCopy;

					if(m_headReadOffset == m_decompressedHead->m_size)
					{
						Buffer* next = m_decompressedHead->m_next;
						m_decompressedHead->m_next = NULL;
						delete m_decompressedHead;
						m_decompressedHead = next;

						if(m_decompressedHead == NULL)
							m_decompressedTail = NULL;
					}
				}
			}
			else
			{
				while(remaining > 0 && m_rawBuffers.size() > 0)
				{
					const Buffer* head = m_rawBuffers.front();
					JELLY_ASSERT(head != NULL);

					JELLY_ASSERT(m_headReadOffset < head->m_size);
					size_t remainingInHeadBuffer = head->m_size - m_headReadOffset;

					size_t toCopy = std::min<size_t>(remaining, remainingInHeadBuffer);

					memcpy(p, head->m_data + m_headReadOffset, toCopy);

					p += toCopy;
					bytesRead += toCopy;
					remaining -= toCopy;
					m_headReadOffset += toCopy;

					if (m_headReadOffset == head->m_size)
						m_rawBuffers.pop_front();
				}
			}

			m_totalBytesRead += bytesRead;

			return bytesRead;
		}
		
		size_t			
		Reader::GetTotalBytesRead() const 
		{
			return m_totalBytesRead;
		}

	}

}