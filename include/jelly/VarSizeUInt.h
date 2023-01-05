#pragma once 

#include <stdint.h>

#include <functional>
#include <type_traits>

#include "ErrorUtils.h"

namespace jelly
{

	namespace VarSizeUInt
	{

		template <typename _T>
		class Encoder
		{
		public:
			static_assert(sizeof(_T) <= sizeof(uint64_t));
			static_assert(std::is_unsigned<_T>::value);

			Encoder()
				: m_size(0)
			{

			}

			Encoder(
				_T							aValue)
				: m_size(0)
			{
				Encode(aValue);
			}

			void
			Encode(
				_T							aValue)
			{
				uint8_t* p = m_buffer;
				m_size = 0;

				for(;;)
				{
					JELLY_ASSERT(m_size < sizeof(m_buffer));

					if(aValue <= 0x7F)
					{
						p[m_size++] = (uint8_t)aValue;
						break;
					}
					else
					{
						p[m_size++] = (uint8_t)((aValue & 0x7F) | 0x80);
						aValue >>= 7;
					}
				}
			}

			// Data access
			const void*		GetBuffer() const { return m_buffer; }
			size_t			GetBufferSize() const { return m_size; }			

		private:

			uint8_t		m_buffer[sizeof(_T) + 2];
			size_t		m_size;
		};

		template <typename _T>
		_T
		Decode(
			std::function<uint8_t()>		aSource)
		{
			static_assert(sizeof(_T) <= sizeof(uint64_t));
			static_assert(std::is_unsigned<_T>::value);

			_T value = 0;
			_T offset = 0;

			for(;;)
			{
				_T i = (_T)aSource();
				
				value |= (i & 0x7F) << offset;
				offset += 7;

				if((i & 0x80) == 0)
					break;
			}

			return value;
		}

	}

}