#pragma once

#if defined(JELLY_ZSTD)

#include <jelly/Compression.h>

namespace jelly
{

	namespace Impl
	{

		class ZstdCompression
			: public Compression::IProvider
		{
		public:
												ZstdCompression();
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
		};

	}

}

#endif // JELLY_ZSTD