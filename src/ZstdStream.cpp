#if defined(JELLY_ZSTD)

#include <assert.h>
#include <stdio.h>

#include <zstd.h>

#include "ZstdStream.h"

namespace jelly
{

	namespace Impl
	{

		namespace ZstdStream
		{

			struct Compressor::Internal
			{
				Internal()
					: m_stream(NULL)
				{
				
				}

				ZSTD_CStream*	m_stream;
			};

			struct Decompressor::Internal
			{
				Internal()
					: m_stream(NULL)
				{

				}

				ZSTD_DStream* m_stream;
			};

			//------------------------------------------------------------------------

			Compressor::Compressor()
				: m_error(false)
			{
				m_internal = new Internal();

				m_internal->m_stream = ZSTD_createCStream();
				assert(m_internal->m_stream != NULL);
			}

			Compressor::~Compressor()
			{
				if(m_internal->m_stream != NULL)
					ZSTD_freeCStream(m_internal->m_stream);

				delete m_internal;
			}

			void	
			Compressor::SetOutputCallback(
				Compression::OutputCallback	aOutputCallback)
			{	
				m_outputCallback = aOutputCallback;
			}

			void
			Compressor::Flush()
			{
				assert(m_internal->m_stream != NULL);
				assert(m_outputCallback);
				assert(!m_error);

				ZSTD_inBuffer inBuffer;
				inBuffer.src = NULL;
				inBuffer.size = 0;
				inBuffer.pos = 0;

				for(;;)
				{
					uint8_t buffer[32 * 1024];

					ZSTD_outBuffer outBuffer;
					outBuffer.dst = buffer;
					outBuffer.size = sizeof(buffer);
					outBuffer.pos = 0;

					size_t result = ZSTD_compressStream2(m_internal->m_stream, &outBuffer, &inBuffer, ZSTD_e_end);
					if(ZSTD_isError(result))
					{
						m_error = true;
						break;
					}

					if (outBuffer.pos > 0)
						m_outputCallback(buffer, outBuffer.pos);					

					if(result == 0)
						break;
				} 
			}

			//------------------------------------------------------------------------

			size_t	
			Compressor::Write(
				const void*				aBuffer,
				size_t					aBufferSize) 
			{
				assert(m_internal->m_stream != NULL);
				assert(m_outputCallback);
				assert(!m_error);

				ZSTD_inBuffer inBuffer;
				inBuffer.src = aBuffer;
				inBuffer.size = aBufferSize;
				inBuffer.pos = 0;				

				while(inBuffer.pos < inBuffer.size)
				{
					uint8_t buffer[32 * 1024];

					ZSTD_outBuffer outBuffer;
					outBuffer.dst = buffer;
					outBuffer.size = sizeof(buffer);
					outBuffer.pos = 0;

					size_t result = ZSTD_compressStream2(m_internal->m_stream, &outBuffer, &inBuffer, ZSTD_e_continue);
					if (ZSTD_isError(result))
					{
						m_error = true;
						return 0;
					}

					if(outBuffer.pos > 0)
						m_outputCallback(buffer, outBuffer.pos);
				}

				return aBufferSize;
			}

			//------------------------------------------------------------------------

			Decompressor::Decompressor()
				: m_error(false)
			{
				m_internal = new Internal();

				m_internal->m_stream = ZSTD_createDStream();
				assert(m_internal->m_stream != NULL);
			}

			Decompressor::~Decompressor()
			{
				if (m_internal->m_stream != NULL)
					ZSTD_freeDStream(m_internal->m_stream);

				delete m_internal;
			}

			void
			Decompressor::SetOutputCallback(
				Compression::OutputCallback	aOutputCallback)
			{
				m_outputCallback = aOutputCallback;
			}
						
			void	
			Decompressor::FeedData(
				const void*					aBuffer,
				size_t						aBufferSize)
			{
				assert(m_internal->m_stream != NULL);
				assert(m_outputCallback);
				assert(!m_error);

				ZSTD_inBuffer inBuffer;
				inBuffer.src = aBuffer;
				inBuffer.size = aBufferSize;
				inBuffer.pos = 0;

				size_t result;
				
				do
				{
					uint8_t buffer[32768];

					ZSTD_outBuffer outBuffer;
					outBuffer.dst = buffer;
					outBuffer.size = sizeof(buffer);
					outBuffer.pos = 0;

					result = ZSTD_decompressStream(m_internal->m_stream, &outBuffer, &inBuffer);
					if(ZSTD_isError(result))
					{
						m_error = true;
						break;
					}

					if (outBuffer.pos > 0)
						m_outputCallback(buffer, outBuffer.pos);
				} while(inBuffer.pos < inBuffer.size);
			}

		}

	}

}

#endif // JELLY_ZSTD