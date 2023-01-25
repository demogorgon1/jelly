#pragma once

#if defined(JELLY_ZSTD)

#include "Compression.h"

namespace jelly
{

	// ZSTD stream compressing and decompressing
	namespace ZstdStream
	{

		class Compressor
			: public Compression::IStreamCompressor
		{
		public:
					Compressor();
					~Compressor();

			// Compression::IStreamCompressor implementation
			void	SetOutputCallback(
						Compression::OutputCallback	aOutputCallback) override;
			void	Flush() override;
			size_t	Write(
						const void*					aBuffer,
						size_t						aBufferSize) override;

		private:

			Compression::OutputCallback	m_outputCallback;
			bool						m_error;
				
			struct Internal;
			Internal*					m_internal;
		};

		class Decompressor
			: public Compression::IStreamDecompressor
		{
		public:
					Decompressor();
					~Decompressor();

			// Compression::IStreamCompressor implementation
			void	SetOutputCallback(
						Compression::OutputCallback	aOutputCallback) override;
			void	FeedData(
						const void*					aBuffer,
						size_t						aBufferSize) override;

		private:

			Compression::OutputCallback	m_outputCallback;
			bool						m_error;
				
			struct Internal;
			Internal*					m_internal;
		};			

	}

}

#endif // JELLY_ZSTD
