#pragma once

#if defined(JELLY_ZSTD)

#include <jelly/Compression.h>

namespace jelly
{

	/**
	 * \brief ZSTD implementation of Compression::IProvider.
	 */
	class ZstdCompression
		: public Compression::IProvider
	{
	public:
											ZstdCompression(
												uint32_t					aBufferCompressionLevel = 0);
		virtual								~ZstdCompression();

		// Compression::IProvider implementation
		Compression::Id						GetId() const override;
		Compression::IStreamCompressor*		CreateStreamCompressor() const override;
		Compression::IStreamDecompressor*	CreateStreamDecompressor() const override;
		IBuffer*							CompressBuffer(
												const IBuffer*				aBuffer) const override;
		IBuffer*							DecompressBuffer(
												const IBuffer*				aBuffer) const override;
		
	private:
			
		uint32_t			m_bufferCompressionLevel;
	};

}

#endif // JELLY_ZSTD