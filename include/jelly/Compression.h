#pragma once

#include "Buffer.h"
#include "BufferWriter.h"
#include "IWriter.h"

namespace jelly
{

	// Interfaces for compression implementation
	namespace Compression
	{

		enum Id : uint8_t
		{
			ID_NO_COMPRESSION,	
			ID_ZSTD	
		};

		typedef std::function<void(const void*, size_t)> OutputCallback;

		/**
		 * \brief Abstract interface to a stream compressor implementing the IWriter interface.
		 */
		class IStreamCompressor
			: public IWriter
		{
		public:		
			virtual							~IStreamCompressor() {}

			//-------------------------------------------------------------------------------
			// Virtual interface

			//! The output callback will be called whenever the compression algorithm wants to output something.
			virtual void					SetOutputCallback(
												OutputCallback	aOutputCallback) = 0;

			//! Tells the compression algorithm to flush any pending data.
			virtual void					Flush() = 0;
		};

		/**
		 * \brief Abstract interface to a stream decompressor.
		 */
		class IStreamDecompressor
		{
		public:
			virtual							~IStreamDecompressor() {}

			//-------------------------------------------------------------------------------
			// Virtual interface

			//! The output callback will be called whenever the algorithm has decompressed something.
			virtual void					SetOutputCallback(
												OutputCallback	aOutputCallback) = 0;

			//! Feed compressed stream data to the decompression algorithm.
			virtual void					FeedData(
												const void*		aBuffer,
												size_t			aBufferSize) = 0;
		};			

		/**
		 * \brief Abstract interface to a compression algorithm.
		 */
		class IProvider
		{
		public:
			virtual							~IProvider() {}

			//! Helper for compressing buffer objects.
			IBuffer*
			CompressBuffer(
				const IBuffer*									aBuffer) const
			{
				std::unique_ptr<IBuffer> compressed = std::make_unique<Buffer<64>>();
				BufferWriter writer(*compressed);
				CompressBuffer(aBuffer->GetPointer(), aBuffer->GetSize(), [&writer](
					const void*	aData,
					size_t		aDataSize)
				{
					writer.Write(aData, aDataSize);
				});
				return compressed.release();
			}

			//! Helper for decompressing buffer objects.
			IBuffer*
			DecompressBuffer(
				const IBuffer*									aBuffer) const
			{
				std::unique_ptr<IBuffer> decompressed = std::make_unique<Buffer<64>>();
				BufferWriter writer(*decompressed);
				DecompressBuffer(aBuffer->GetPointer(), aBuffer->GetSize(), [&writer](
					const void* aData,
					size_t		aDataSize)
				{
					writer.Write(aData, aDataSize);
				});
				return decompressed.release();
			}

			//-------------------------------------------------------------------------------
			// Virtual interface

			//! Returns the id of the compression algorithm
			virtual Id						GetId() const = 0;

			//! Creates a stream compressor object
			virtual IStreamCompressor*		CreateStreamCompressor() const = 0;

			//! Creates a stream decompress object
			virtual IStreamDecompressor*	CreateStreamDecompressor() const = 0;

			//! Compress a single buffer
			virtual void					CompressBuffer(
												const void*		aBuffer,
												size_t			aBufferSize,
												OutputCallback	aOutputCallback) const = 0;

			//! Decompress a single previously compressed buffer
			virtual void					DecompressBuffer(
												const void*		aBuffer,
												size_t			aBufferSize,
												OutputCallback	aOutputCallback) const = 0;
		};

	}

}