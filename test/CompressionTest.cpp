// Test compression implementations

#include <jelly/API.h>

namespace jelly
{

	namespace Test
	{

		namespace
		{

			template<typename _ProviderType>
			void
			_TestStreamCompressionData(	
				const void*		aData,
				size_t			aDataSize,
				bool			aDecompressOneByteAtATime,
				bool			aCompressOneByteAtATime,
				bool			aFlushAfterEachByte)
			{
				_ProviderType compression;

				std::unique_ptr<Compression::IStreamCompressor> compressor(compression.CreateStreamCompressor());
				std::unique_ptr<Compression::IStreamDecompressor> decompressor(compression.CreateStreamDecompressor());

				compressor->SetOutputCallback([aDecompressOneByteAtATime, &decompressor](
					const void* aBuffer,
					size_t		aBufferSize)
				{
					if(aDecompressOneByteAtATime)
					{
						for(size_t i = 0; i < aBufferSize; i++)
							decompressor->FeedData((const uint8_t*)aBuffer + i, 1);
					}
					else
					{
						decompressor->FeedData(aBuffer, aBufferSize);
					}
				});

				const uint8_t* comparePointer = (const uint8_t*)aData;
				size_t compareRemaining = aDataSize;

				decompressor->SetOutputCallback([&comparePointer, &compareRemaining](
					const void* aBuffer,
					size_t		aBufferSize)
				{
					JELLY_ALWAYS_ASSERT(aBufferSize <= compareRemaining);
					JELLY_ALWAYS_ASSERT(memcmp(aBuffer, comparePointer, aBufferSize) == 0);
					comparePointer += aBufferSize;
					compareRemaining -= aBufferSize;
				});

				if(aCompressOneByteAtATime)
				{
					for (size_t i = 0; i < aDataSize; i++)
					{
						compressor->Write((const uint8_t*)aData + i, 1);

						if(aFlushAfterEachByte)
							compressor->Flush();
					}

					if (!aFlushAfterEachByte)
						compressor->Flush();
				}
				else
				{
					compressor->Write(aData, aDataSize);
					compressor->Flush();
				}

				JELLY_ALWAYS_ASSERT(compareRemaining == 0);
			}

			template<typename _ProviderType>
			void
			_TestStreamCompressionString(	
				const char*		aString)
			{
				// Try different permutations of how to feed data to compressor/decompressor
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), false, false, false);
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), true, false, false);
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), true, true, false);
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), true, true, true);
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), false, true, false);
				_TestStreamCompressionData<_ProviderType>(aString, strlen(aString), false, true, true);
			}

			template<typename _ProviderType>
			void
			_TestBufferCompressionString(
				const char*		aString)
			{
				_ProviderType compression;

				size_t strLen = strlen(aString); 

				Buffer<1> sourceBuffer;
				sourceBuffer.SetSize(strLen);
				memcpy(sourceBuffer.GetPointer(), aString, strLen);

				std::unique_ptr<IBuffer> compressedBuffer(compression.CompressBuffer(&sourceBuffer));
				std::unique_ptr<IBuffer> uncompressedBuffer(compression.DecompressBuffer(compressedBuffer.get()));

				JELLY_ALWAYS_ASSERT(sourceBuffer == *uncompressedBuffer);
			}

			template<typename _ProviderType>
			void	
			_TestCompressionProvider()
			{
				// Try a couple strings
				_TestStreamCompressionString<_ProviderType>("");
				_TestStreamCompressionString<_ProviderType>("hello");
				
				// Make a very long string
				{
					std::stringstream longString;
					for (int i = 0; i < 200; i++)
					{
						char buffer[64];
						JELLY_ALWAYS_ASSERT((size_t)snprintf(buffer, sizeof(buffer), "%d", i * 349104) <= sizeof(buffer));
						longString << buffer;
					}

					std::string t = longString.str();
					_TestStreamCompressionString<_ProviderType>(t.c_str());
				}

				// Make a big buffer that's definitely larger than any internal temporary buffer
				{
					static const size_t SIZE = 1024 * 1024;
					uint8_t* bigBuffer = new uint8_t[SIZE];
					try
					{
						uint8_t byte = 0;
						for(size_t i = 0; i < SIZE; i++)
							bigBuffer[i] = byte++;

						_TestStreamCompressionData<_ProviderType>(bigBuffer, SIZE, false, false, false);
					}
					catch(...)
					{
						JELLY_ALWAYS_ASSERT(false);
					}

					delete [] bigBuffer;
				}

				// Test buffer compression
				{
					_TestBufferCompressionString<_ProviderType>("");
					_TestBufferCompressionString<_ProviderType>("hello");
				}
			}

		}

		namespace CompressionTest
		{

			void		
			Run()
			{
				#if defined(JELLY_ZSTD)
					_TestCompressionProvider<ZstdCompression>();
				#endif
			}

		}

	}

}