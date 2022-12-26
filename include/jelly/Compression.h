#pragma once

#include <functional>

#include "IWriter.h"

namespace jelly
{

	// Interfaces for compression implementation
	namespace Compression
	{

		typedef std::function<void(const void*, size_t)> OutputCallback;

		class IStreamCompressor
			: public IWriter
		{
		public:		
			virtual							~IStreamCompressor() {}

			// Virtual interface
			virtual void					SetOutputCallback(
												OutputCallback	aOutputCallback) = 0;
			virtual void					Flush() = 0;
		};

		class IStreamDecompressor
		{
		public:
			virtual							~IStreamDecompressor() {}

			// Virtual interface
			virtual void					SetOutputCallback(
												OutputCallback	aOutputCallback) = 0;
			virtual void					FeedData(
												const void*		aBuffer,
												size_t			aBufferSize) = 0;
		};			

		class IProvider
		{
		public:
			virtual							~IProvider() {}

			// Virtual interface
			virtual IStreamCompressor*		CreateStreamCompressor() const = 0;
			virtual IStreamDecompressor*	CreateStreamDecompressor() const = 0;
			virtual void					CompressBuffer(
												const void*		aBuffer,
												size_t			aBufferSize,
												OutputCallback	aOutputCallback) const = 0;
			virtual void					DecompressBuffer(
												const void*		aBuffer,
												size_t			aBufferSize,
												OutputCallback	aOutputCallback) const = 0;
		};

	}

}