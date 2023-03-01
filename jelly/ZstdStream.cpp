#include <jelly/Base.h>

#if defined(JELLY_ZSTD)

#include <zstd.h>

#include <jelly/ErrorUtils.h>
#include <jelly/ZstdStream.h>

namespace jelly
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
			: m_totalBytesWritten(0)
		{
			m_internal = new Internal();

			m_internal->m_stream = ZSTD_createCStream();
			JELLY_CHECK(m_internal->m_stream != NULL, Exception::ERROR_ZSTD_STREAM_FAILED_TO_CREATE_COMPRESSOR);
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
			JELLY_ASSERT(m_internal->m_stream != NULL);
			JELLY_ASSERT(m_outputCallback);

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
				JELLY_CHECK(ZSTD_isError(result) == 0, Exception::ERROR_ZSTD_STREAM_FAILED_TO_FLUSH, "Msg=%s", ZSTD_getErrorName(result));

				if (outBuffer.pos > 0)
					m_outputCallback(buffer, outBuffer.pos);					

				if(result == 0)
					break;
			} 
		}

		//------------------------------------------------------------------------

		void
		Compressor::Write(
			const void*				aBuffer,
			size_t					aBufferSize) 
		{
			JELLY_ASSERT(m_internal->m_stream != NULL);
			JELLY_ASSERT(m_outputCallback);

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
				JELLY_CHECK(ZSTD_isError(result) == 0, Exception::ERROR_ZSTD_STREAM_FAILED_TO_WRITE, "Msg=%s", ZSTD_getErrorName(result));

				if(outBuffer.pos > 0)
					m_outputCallback(buffer, outBuffer.pos);
			}

			m_totalBytesWritten += aBufferSize;
		}

		size_t	
		Compressor::GetTotalBytesWritten() const
		{
			return m_totalBytesWritten;
		}

		//------------------------------------------------------------------------

		Decompressor::Decompressor()
		{
			m_internal = new Internal();

			m_internal->m_stream = ZSTD_createDStream();
			JELLY_CHECK(m_internal->m_stream != NULL, Exception::ERROR_ZSTD_STREAM_FAILED_TO_CREATE_DECOMPRESSOR);
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
			JELLY_ASSERT(m_internal->m_stream != NULL);
			JELLY_ASSERT(m_outputCallback);

			ZSTD_inBuffer inBuffer;
			inBuffer.src = aBuffer;
			inBuffer.size = aBufferSize;
			inBuffer.pos = 0;

			do
			{
				uint8_t buffer[32768];

				ZSTD_outBuffer outBuffer;
				outBuffer.dst = buffer;
				outBuffer.size = sizeof(buffer);
				outBuffer.pos = 0;

				size_t result = ZSTD_decompressStream(m_internal->m_stream, &outBuffer, &inBuffer);
				JELLY_CHECK(ZSTD_isError(result) == 0, Exception::ERROR_ZSTD_STREAM_FAILED_TO_READ, "Msg=%s", ZSTD_getErrorName(result));

				if (outBuffer.pos > 0)
					m_outputCallback(buffer, outBuffer.pos);
			} while(inBuffer.pos < inBuffer.size);
		}

	}

}

#endif // JELLY_ZSTD