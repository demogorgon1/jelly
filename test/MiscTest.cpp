#include <limits>
#include <type_traits>

#include <jelly/Blob.h>
#include <jelly/BufferReader.h>
#include <jelly/VarSizeUInt.h>
#include <jelly/ZstdCompression.h>

namespace jelly
{

	namespace Test
	{

		namespace 
		{

			template <typename _T>
			void
			_TestVarSizeUIntValue(
				_T			aValue)
			{
				VarSizeUInt::Encoder<_T> encoder(aValue);
				
				if(aValue <= 0x7F)
					JELLY_ASSERT(encoder.GetBufferSize() == 1);
				else 
					JELLY_ASSERT(encoder.GetBufferSize() > 1);

				const uint8_t* p = (const uint8_t*)encoder.GetBuffer();
				size_t i = 0;

				_T decoded = VarSizeUInt::Decode<_T>([&]() -> uint8_t 
				{ 
					JELLY_ASSERT(i < encoder.GetBufferSize()); 
					return p[i++]; 
				});

				JELLY_ASSERT(aValue == decoded);
			}

			template <typename _T>
			void
			_TestVarSizeUInt()
			{
				static_assert(std::is_unsigned<_T>());

				_TestVarSizeUIntValue<_T>(0);

				for(_T i = 1; i < 10; i++)
					_TestVarSizeUIntValue<_T>(std::numeric_limits<_T>::max() / i);
			}

		}

		namespace MiscTest
		{

			void		
			Run()
			{	
				// Var size uints
				{
					{
						_TestVarSizeUInt<uint8_t>();
						_TestVarSizeUInt<uint16_t>();
						_TestVarSizeUInt<uint32_t>();
						_TestVarSizeUInt<uint64_t>();
						_TestVarSizeUInt<size_t>();
					}

					{
						VarSizeUInt::Encoder<uint32_t> encoder(UINT32_MAX);
						JELLY_ASSERT(encoder.GetBufferSize() == 5);
					}

					{
						VarSizeUInt::Encoder<uint64_t> encoder(UINT64_MAX);
						JELLY_ASSERT(encoder.GetBufferSize() == 10);
					}
				}

				// Blobs
				{
					Blob blob;
					blob.SetSize(5);
					memcpy(blob.GetBuffer(), "hello", 5);

					{
						BufferWriter<100> writer;
						JELLY_ASSERT(blob.Write(&writer, NULL));

						BufferReader reader(writer.GetBuffer(), writer.GetBufferSize());
						Blob blob2;
						JELLY_ASSERT(blob2.Read(&reader, NULL));
						JELLY_ASSERT(blob2 == blob);
						JELLY_ASSERT(blob2.GetSize() == 5);
						JELLY_ASSERT(memcmp(blob2.GetBuffer(), "hello", 5) == 0);
					}

					#if defined(JELLY_ZSTD)
						// First compress the uncompressible small blob
						{
							ZstdCompression compressionProvider;

							BufferWriter<100> writer;
							JELLY_ASSERT(blob.Write(&writer, &compressionProvider));

							BufferReader reader(writer.GetBuffer(), writer.GetBufferSize());
							Blob blob2;
							JELLY_ASSERT(blob2.Read(&reader, &compressionProvider));
							JELLY_ASSERT(blob2 == blob);
							JELLY_ASSERT(blob2.GetSize() == 5);
							JELLY_ASSERT(memcmp(blob2.GetBuffer(), "hello", 5) == 0);
						}

						// Make a blob that can actually be compressed and try with that instead
						{
							ZstdCompression compressionProvider;

							blob.SetSize(100); // Gonna have a lot of 0s

							BufferWriter<100> writer;
							JELLY_ASSERT(blob.Write(&writer, &compressionProvider));

							BufferReader reader(writer.GetBuffer(), writer.GetBufferSize());
							Blob blob2;
							JELLY_ASSERT(blob2.Read(&reader, &compressionProvider));
							JELLY_ASSERT(blob2 == blob);
							JELLY_ASSERT(blob2.GetSize() == 100);
							JELLY_ASSERT(memcmp(blob2.GetBuffer(), "hello", 5) == 0); // First 5 bytes should be hello

							for(size_t i = 5; i < 100; i++)
							{
								// Remaining bytes should be 0
								JELLY_ASSERT(((const uint8_t*)blob2.GetBuffer())[i] == 0);
							}
						}
					#endif
				}
			}

		}

	}

}