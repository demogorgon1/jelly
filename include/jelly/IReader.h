#pragma once

#include "VarSizeUInt.h"

namespace jelly
{

	// Abstract binary reader interface
	class IReader
	{
	public:
		virtual			~IReader() {}

		template <typename _T>
		bool
		ReadUInt(
			_T&						aOutValue,
			size_t*					aOutSize = NULL)
		{
			bool readError = false;
			size_t size = 0;

			aOutValue = VarSizeUInt::Decode<_T>([&]() -> uint8_t
			{
				uint8_t byte;
				if(!ReadPOD<uint8_t>(byte))
				{
					readError = true;
					return 0;
				}

				size++;
				return byte;
			});

			if(aOutSize != NULL)
				*aOutSize = size;

			return !readError;
		}

		template <typename _T>
		bool
		ReadPOD(
			_T&						aOutValue)
		{
			return Read(&aOutValue, sizeof(_T)) == sizeof(_T);
		}

		// Virtual interface
		virtual size_t	Read(
							void*	aBuffer,
							size_t	aBufferSize) = 0;
	};

}