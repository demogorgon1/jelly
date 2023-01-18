#pragma once

#include "Buffer.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Compression.h"
#include "IBuffer.h"

namespace jelly
{

	template<size_t _StaticSize = 1>
	class Blob
	{
	public:
		Blob()
		{

		}

		~Blob()
		{

		}

		void
		ToBuffer(
			const Compression::IProvider*	aCompression,	
			IBuffer&						aOut) const
		{
			JELLY_ASSERT(aOut.GetSize() == 0);

			BufferWriter writer(aOut);
						
			if(aCompression != NULL)
			{
				if(!writer.WritePOD<Compression::Id>(aCompression->GetId()))
					JELLY_ASSERT(false);

				if(!writer.WriteUInt<size_t>(m_buffer.GetSize())) // Uncompressed size (compressed size is implied by total buffer size)
					JELLY_ASSERT(false);

				aCompression->CompressBuffer(m_buffer.GetPointer(), m_buffer.GetSize(), [&](
					const void* aCompressedData,
					size_t		aCompressedDataSize)
				{
					if(writer.Write(aCompressedData, aCompressedDataSize) != aCompressedDataSize)
						JELLY_ASSERT(false);
				});
			}
			else
			{
				writer.WritePOD<Compression::Id>(Compression::ID_NO_COMPRESSION);

				if(writer.Write(m_buffer.GetPointer(), m_buffer.GetSize()) != m_buffer.GetSize())
					JELLY_ASSERT(false);
			}
		}

		void
		FromBuffer(
			const Compression::IProvider*	aCompression,
			const IBuffer&					aBuffer) 
		{
			JELLY_ASSERT(m_buffer.GetSize() == 0);

			BufferReader reader(aBuffer.GetPointer(), aBuffer.GetSize());
			BufferWriter writer(m_buffer);

			Compression::Id blobCompression;
			if(!reader.ReadPOD<Compression::Id>(blobCompression))
				JELLY_ASSERT(false);

			if(aCompression != NULL && blobCompression != Compression::ID_NO_COMPRESSION)
			{
				JELLY_CHECK(blobCompression == aCompression->GetId(), "Compression algorithm mismatch.");

				size_t uncompressedSize;
				if(!reader.ReadUInt<size_t>(uncompressedSize))
					JELLY_ASSERT(false);

				aCompression->DecompressBuffer(reader.GetCurrentPointer(), reader.GetRemainingSize(), [&](
					const void*	aUncompressedData,
					size_t		aUncompressedDataSize)
				{
					if(writer.Write(aUncompressedData, aUncompressedDataSize) != aUncompressedDataSize)
						JELLY_ASSERT(false);
				});
			}
			else
			{
				JELLY_CHECK(blobCompression == Compression::ID_NO_COMPRESSION, "Unable to decompress blob.");

				size_t remainingSize = reader.GetRemainingSize();
				if(writer.Write(reader.GetCurrentPointer(), remainingSize) != remainingSize)
					JELLY_ASSERT(false);
			}
		}

		bool		
		operator==(
			const Blob&						aOther) const 
		{
			return m_buffer == aOther.m_buffer;
		}

		Blob&
		operator=(
			const Blob&						aOther) 
		{
			m_buffer = aOther.m_buffer;
			return *this;
		}

		// Data access
		const Buffer<_StaticSize>&	GetBuffer() const { return m_buffer; }
		Buffer<_StaticSize>&		GetBuffer() { return m_buffer; }

	private:
		
		Buffer<_StaticSize>		m_buffer;
	};

}