#if defined(JELLY_ZSTD)

#include <assert.h>

#include "ZstdCompression.h"
#include "ZstdStream.h"

namespace jelly
{

	ZstdCompression::ZstdCompression()
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
		ZstdStream::Compressor compressor;
		compressor.SetOutputCallback(aOutputCallback);
		size_t bytes = compressor.Write(aBuffer, aBufferSize);
		assert(bytes == aBufferSize);
		compressor.Flush();
	}
		
	void					
	ZstdCompression::DecompressBuffer(
		const void*					aBuffer,
		size_t						aBufferSize,
		Compression::OutputCallback	aOutputCallback) const
	{
		ZstdStream::Decompressor decompressor;
		decompressor.SetOutputCallback(aOutputCallback);
		decompressor.FeedData(aBuffer, aBufferSize);
	}

}

#endif // JELLY_ZSTD