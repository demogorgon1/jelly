#pragma once

#include "BufferWriter.h"
#include "Compression.h"
#include "ErrorUtils.h"
#include "IReader.h"
#include "IWriter.h"

namespace jelly
{

	class Blob
	{
	public:
		Blob()
			: m_size(0)
			, m_data(NULL)
		{

		}

		~Blob()
		{
			if(m_data != NULL)
				delete [] m_data;
		}

		void
		SetSize(
			size_t							aSize)
		{
			_Realloc(aSize);
		}

		const void*
		GetBuffer() const
		{
			return m_data;
		}

		void*
		GetBuffer()
		{
			return m_data;
		}

		bool
		Write(
			IWriter*						aWriter,
			const Compression::IProvider*	aCompression) const
		{
			if(!aWriter->WriteUInt<size_t>(m_size))
				return false;

			if(aCompression != NULL)
			{				
				BufferWriter<16834> compressed;

				aCompression->CompressBuffer(m_data, m_size, [&compressed](
					const void*	aCompressedData,
					size_t		aCompressedDataSize)
				{
					compressed.Write(aCompressedData, aCompressedDataSize);
				});

				if(compressed.GetBufferSize() < m_size)
				{
					// Write compressed blob
					if (!aWriter->WriteUInt<size_t>(compressed.GetBufferSize()))
						return false;

					if(aWriter->Write(compressed.GetBuffer(), compressed.GetBufferSize()) != compressed.GetBufferSize())
						return false;
				}
				else
				{
					// Write uncompressed blob as compression didn't actually reduce the size
					if (!aWriter->WriteUInt<size_t>(m_size))
						return false;

					if (aWriter->Write(m_data, m_size) != m_size)
						return false;
				}
			}
			else
			{
				if (!aWriter->WriteUInt<size_t>(m_size))
					return false;

				if (aWriter->Write(m_data, m_size) != m_size)
					return false;
			}

			return true;
		}

		bool
		Read(
			IReader*						aReader,
			const Compression::IProvider*	aCompression)
		{
			size_t uncompressedSize;
			if(!aReader->ReadUInt<size_t>(uncompressedSize))
				return false;

			_Realloc(uncompressedSize);

			size_t compressedSize;
			if (!aReader->ReadUInt<size_t>(compressedSize))
				return false;

			JELLY_CHECK(compressedSize <= m_size, "Invalid compressed blob size.");

			if(compressedSize < m_size)
			{
				JELLY_CHECK(aCompression != NULL, "Unable to uncompressed blob.");

				uint8_t* compressed = new uint8_t[compressedSize];

				try
				{
					if(aReader->Read(compressed, compressedSize) != compressedSize)
					{
						delete[] compressed;
						return false;
					}

					size_t writeOffset = 0;

					aCompression->DecompressBuffer(compressed, compressedSize, [&](
						const void*		aDecompressedData,
						size_t			aDecompressedDataSize)
					{
						JELLY_CHECK(writeOffset + aDecompressedDataSize <= m_size, "Uncompressing blob caused buffer overflow.");

						memcpy(m_data + writeOffset, aDecompressedData, aDecompressedDataSize);

						writeOffset += aDecompressedDataSize;
					});
				}
				catch(...)
				{
					delete [] compressed;
					return false;
				}

				delete[] compressed;
			}
			else
			{
				if(aReader->Read(m_data, m_size) != m_size)
					return false;
			}

			return true;
		}

		bool		
		operator==(
			const Blob&						aOther) const 
		{
			if(GetSize() != aOther.GetSize())
				return false;

			return memcmp(GetBuffer(), aOther.GetBuffer(), GetSize()) == 0;
		}
		
		bool		
		IsSet() const 
		{ 
			return m_data != NULL;
		}
		
		void		
		Reset() 
		{
			if (m_data != NULL)
				delete[] m_data;

			m_data = NULL;
			m_size = 0;
		}
		
		size_t		
		GetSize() const 
		{ 
			return m_size;
		}
		
		size_t		
		GetStoredSize() const 
		{ 
			return m_size;
		}
		
		void		
		Move(
			Blob&							aOther) 
		{ 
			Reset();

			m_data = aOther.m_data;
			m_size = aOther.m_size;

			aOther.m_data = NULL;
			aOther.m_size = 0;
		}
		
		void		
		Copy(
			const Blob&						aOther) 
		{
			if(!aOther.IsSet())
			{
				Reset();
			}
			else
			{
				_Realloc(aOther.GetSize());

				memcpy(m_data, aOther.GetBuffer(), aOther.GetSize());
			}
		}
		
		void		
		Delete() 
		{		
			if(m_data != NULL)
			{
				delete [] m_data;
				m_data = NULL;
			}

			m_size = 0;
		}

	private:

		uint8_t*	m_data;
		size_t		m_size;

		void
		_Realloc(
			size_t							aNewSize)
		{
			if(aNewSize == m_size)
				return;

			uint8_t* newData = new uint8_t[aNewSize];

			if(m_data != NULL)
			{
				if(aNewSize > m_size)
				{
					if(m_size > 0)
						memcpy(newData, m_data, m_size);

					memset(newData + m_size, 0, aNewSize - m_size);
				}
				else
				{
					memcpy(newData, m_data, aNewSize);
				}

				delete[] m_data;
			}
			else
			{
				memset(newData, 0, aNewSize);
			}

			m_data = newData;
			m_size = aNewSize;
		}
	};

}