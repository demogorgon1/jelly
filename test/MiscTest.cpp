#include <type_traits>

#include <jelly/VarSizeUInt.h>

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

				for(size_t j = 0; j < encoder.GetBufferSize(); j++)
					printf("%02X ", p[j]);

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

		}

	}

}