#include <jelly/Base.h>

#if defined(JELLY_ZSTD)

#include <zstd.h>

#include <jelly/ErrorUtils.h>
#include <jelly/ZstdCompression.h>
#include <jelly/ZstdStream.h>

#define MAX_UNCOMPRESSED_BUFFER_SIZE	1024 * 1024

namespace jelly
{

	ZstdCompression::ZstdCompression(
		uint32_t					aBufferCompressionLevel)
		: m_bufferCompressionLevel(aBufferCompressionLevel != 0 ? aBufferCompressionLevel : (uint32_t)ZSTD_CLEVEL_DEFAULT)
	{

	}

	ZstdCompression::~ZstdCompression()
	{

	}

	//---------------------------------------------------------------------------------------------

	Compression::IStreamCompressor*
	ZstdCompression::CreateStreamCompressor() const 
	{
		return new ZstdStream::Compressor();
	}
		
	Compression::IStreamDecompressor*
	ZstdCompression::CreateStreamDecompressor() const 
	{
		return new ZstdStream::Decompressor();
	}
		
	void					
	ZstdCompression::CompressBuffer(
		const void*					aBuffer,
		size_t						aBufferSize,
		Compression::OutputCallback	aOutputCallback) const
	{
		JELLY_CHECK(aBufferSize <= MAX_UNCOMPRESSED_BUFFER_SIZE, "Buffer exceeds maximum uncompressed buffer size.");

		uint8_t staticBuffer[32768];
		uint8_t* outputBuffer = staticBuffer;
		size_t outputBufferSize = sizeof(staticBuffer);

		size_t compressBound = ZSTD_compressBound(aBufferSize) + sizeof(uint32_t);

		if(compressBound > outputBufferSize)
		{
			outputBuffer = new uint8_t[compressBound];
			outputBufferSize = compressBound;
		}

		try
		{
			uint8_t* compressOutputPtr = outputBuffer + sizeof(uint32_t);
			size_t compressOutputSize = outputBufferSize - sizeof(uint32_t);

			size_t result = ZSTD_compress(compressOutputPtr, compressOutputSize, aBuffer, aBufferSize, (int)m_bufferCompressionLevel);
			JELLY_CHECK(!ZSTD_isError(result), "ZSTD_compress() failed.");

			*((uint32_t*)outputBuffer) = (uint32_t)aBufferSize; // Store uncompressed size

			aOutputCallback(outputBuffer, result + sizeof(uint32_t));
		}
		catch(...)
		{
			JELLY_FATAL_ERROR("Unhandled exception when compressing buffer.");
		}

		if(outputBuffer != staticBuffer)
			delete [] outputBuffer;
	}
		
	void					
	ZstdCompression::DecompressBuffer(
		const void*					aBuffer,
		size_t						aBufferSize,
		Compression::OutputCallback	aOutputCallback) const
	{
		JELLY_CHECK(aBufferSize >= sizeof(uint32_t), "Invalid compressed buffer.");

		uint8_t staticBuffer[32768];
		uint8_t* outputBuffer = staticBuffer;
		size_t outputBufferSize = sizeof(staticBuffer);

		size_t uncompressedSize = (size_t)*((uint32_t*)aBuffer);

		if(uncompressedSize == 0)
		{
			aOutputCallback(NULL, 0);
			return;
		}

		JELLY_CHECK(uncompressedSize <= MAX_UNCOMPRESSED_BUFFER_SIZE, "Unable to decompress buffer as uncompressed size is too large.");

		if (uncompressedSize > outputBufferSize)
		{
			outputBuffer = new uint8_t[uncompressedSize];
			outputBufferSize = uncompressedSize;
		}

		try
		{
			const uint8_t* decompressInputPtr = (const uint8_t*)aBuffer + sizeof(uint32_t);
			size_t decompressInputSize = aBufferSize - sizeof(uint32_t);

			size_t result = ZSTD_decompress(outputBuffer, outputBufferSize, decompressInputPtr, decompressInputSize);
			JELLY_CHECK(!ZSTD_isError(result), "ZSTD_decompress() failed.");

			aOutputCallback(outputBuffer, result);
		}
		catch(...)
		{
			JELLY_FATAL_ERROR("Unhandled exception when decompressing buffer.");
		}

		if(outputBuffer != staticBuffer)
			delete [] outputBuffer;
	}

}

#endif // JELLY_ZSTD