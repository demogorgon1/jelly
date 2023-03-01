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

	Compression::Id			
	ZstdCompression::GetId() const 
	{
		return Compression::ID_ZSTD;
	}

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
		
	IBuffer* 
	ZstdCompression::CompressBuffer(
		const IBuffer*				aBuffer) const 
	{
		JELLY_CHECK(aBuffer->GetSize() <= MAX_UNCOMPRESSED_BUFFER_SIZE, Exception::ERROR_ZSTD_BUFFER_TOO_LARGE_TO_COMPRESS, "Size=%zu;Max=%zu", aBuffer->GetSize(), MAX_UNCOMPRESSED_BUFFER_SIZE);

		size_t compressBound = ZSTD_compressBound(aBuffer->GetSize()) + sizeof(uint32_t);

		std::unique_ptr<Buffer<1>> compressed = std::make_unique<Buffer<1>>();
		compressed->SetSize(compressBound);

		uint8_t* outputBuffer = (uint8_t*)compressed->GetPointer();
		uint8_t* compressOutputPtr = outputBuffer + sizeof(uint32_t);
		size_t compressOutputSize = compressBound - sizeof(uint32_t);

		size_t result = ZSTD_compress(compressOutputPtr, compressOutputSize, aBuffer->GetPointer(), aBuffer->GetSize(), (int)m_bufferCompressionLevel);
		JELLY_CHECK(ZSTD_isError(result) == 0, Exception::ERROR_ZSTD_BUFFER_COMPRESSION_FAILED, "Size=%zu;Msg=%s", aBuffer->GetSize(), ZSTD_getErrorName(result));

		*((uint32_t*)outputBuffer) = (uint32_t)aBuffer->GetSize(); // Store uncompressed size

		compressed->SetSize(result + sizeof(uint32_t));

		return compressed.release();
	}
	
	IBuffer* 
	ZstdCompression::DecompressBuffer(
		const IBuffer*				aBuffer) const 
	{
		JELLY_CHECK(aBuffer->GetSize() >= sizeof(uint32_t), Exception::ERROR_ZSTD_BUFFER_INVALID);

		std::unique_ptr<Buffer<1>> uncompressed = std::make_unique<Buffer<1>>();
		size_t uncompressedSize = (size_t)*((uint32_t*)aBuffer->GetPointer());
		if (uncompressedSize == 0)
			return uncompressed.release();

		JELLY_CHECK(uncompressedSize <= MAX_UNCOMPRESSED_BUFFER_SIZE, Exception::ERROR_ZSTD_BUFFER_TOO_LARGE_TO_UNCOMPRESS, "Size=%zu;Max=%zu", aBuffer->GetSize(), MAX_UNCOMPRESSED_BUFFER_SIZE);
		uncompressed->SetSize(uncompressedSize);

		const uint8_t* decompressInputPtr = (const uint8_t*)aBuffer->GetPointer() + sizeof(uint32_t);
		size_t decompressInputSize = aBuffer->GetSize() - sizeof(uint32_t);

		size_t result = ZSTD_decompress(uncompressed->GetPointer(), uncompressedSize, decompressInputPtr, decompressInputSize);
		JELLY_CHECK(ZSTD_isError(result) == 0, Exception::ERROR_ZSTD_BUFFER_DECOMPRESSION_FAILED, "Compressed=%zu;Uncompressed=%zu;Msg=%s", aBuffer->GetSize(), uncompressedSize, ZSTD_getErrorName(result));

		return uncompressed.release();
	}

}

#endif // JELLY_ZSTD