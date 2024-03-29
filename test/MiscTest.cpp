#include <limits>

#include <jelly/API.h>

#include <jelly/StringUtils.h>

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
					JELLY_ALWAYS_ASSERT(encoder.GetBufferSize() == 1);
				else 
					JELLY_ALWAYS_ASSERT(encoder.GetBufferSize() > 1);

				const uint8_t* p = (const uint8_t*)encoder.GetBuffer();
				size_t i = 0;

				_T decoded = VarSizeUInt::Decode<_T>([&]() -> uint8_t 
				{ 
					JELLY_ALWAYS_ASSERT(i < encoder.GetBufferSize()); 
					return p[i++]; 
				});

				JELLY_UNUSED(decoded);
				JELLY_ALWAYS_ASSERT(aValue == decoded);
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
				// String utils
				{
					JELLY_ALWAYS_ASSERT(StringUtils::ParseBool("0") == false);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseBool("1") == true);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseUInt32("0") == 0);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseUInt32("123456") == 123456);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseUInt32("4294967295") == UINT32_MAX);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("0") == 0);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("100") == 100);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("1s") == 1000);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("1s500ms") == 1500);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("1m30s") == 60 * 1000 + 30 * 1000);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseInterval("1h30m10s") == 60 * 60 * 1000 + 30 * 60 * 1000 + 10 * 1000);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseSize("1KB") == 1024);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseSize("1 KB") == 1024);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseSize("1k") == 1024);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseSize("2MB") == 1024 * 1024 * 2);
					JELLY_ALWAYS_ASSERT(StringUtils::ParseSize("1G") == 1024 * 1024 * 1024);
				}

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
						JELLY_ALWAYS_ASSERT(encoder.GetBufferSize() == 5);
					}

					{
						VarSizeUInt::Encoder<uint64_t> encoder(UINT64_MAX);
						JELLY_ALWAYS_ASSERT(encoder.GetBufferSize() == 10);
					}
				}
			}

		}

	}

}