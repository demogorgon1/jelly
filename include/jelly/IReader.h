#pragma once

#include "VarSizeUInt.h"

namespace jelly
{

	/**
	 * \brief Abstract binary reader interface.
	 * 
	 * \see IWriter
	 */
	class IReader
	{
	public:
		virtual			~IReader() {}

		//! Reads a variably sized unsigned integer. Optionally returns the size of it.
		template <typename _T>
		bool
		ReadUInt(
			_T&						aOutValue,
			size_t*					aOutSize = NULL)
		{
			#if defined(JELLY_VAR_SIZE_UINTS)
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
			#else
				if(aOutSize != NULL)
					*aOutSize = sizeof(_T);

				return ReadPOD(aOutValue);
			#endif
		}

		//! Reads a POD type.
		template <typename _T>
		bool
		ReadPOD(
			_T&						aOutValue)
		{
			return Read(&aOutValue, sizeof(_T)) == sizeof(_T);
		}

		//! Helper for reading a vector of objects.
		template <typename _T>
		bool
		ReadObjects(
			std::vector<_T>&		aOut,
			size_t					aMaxCount = 1024)
		{
			size_t count;
			if(!ReadUInt(count))
				return false;

			if(count > aMaxCount)	
				return false;

			aOut.resize(count);

			for(_T& t : aOut)
			{
				if(!t.Read(this))
					return false;
			}

			return true;
		}

		//---------------------------------------------------------------------------
		// Virtual interface

		//! Reads into a buffer. Returns number of bytes actually read (0 if end has been reached).
		virtual size_t	Read(
							void*	aBuffer,
							size_t	aBufferSize) = 0;

		//! Returns total number of bytes read.
		virtual size_t	GetTotalBytesRead() const = 0;
	};

}