#pragma once

#if defined(JELLY_ZSTD)

#include <jelly/Compression.h>

namespace jelly
{

	class ZstdCompression
		: public Compression::IProvider
	{
	public:
											ZstdCompression(
												uint32_t					aBufferCompressionLevel = 0);
		virtual								~ZstdCompression();

		// Compression::IProvider implementation
		Compression::IStreamCompressor*		CreateStreamCompressor() const override;
		Compression::IStreamDecompressor*	CreateStreamDecompressor() const override;
		void								CompressBuffer(
												const void*					aBuffer,
												size_t						aBufferSize,
												Compression::OutputCallback	aOutputCallback) const override;
		void								DecompressBuffer(
												const void*					aBuffer,
												size_t						aBufferSize,
												Compression::OutputCallback	aOutputCallback) const override;

	private:
			
		uint32_t			m_bufferCompressionLevel;
	};

}

#endif // JELLY_ZSTD