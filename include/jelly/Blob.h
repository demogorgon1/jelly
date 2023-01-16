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

			// FIXME: some kind of compression method fingerprint
			
			if(aCompression != NULL)
			{
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

			// FIXME: verify some kind of compression method fingerprint

			if(aCompression != NULL)
			{
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
				if(writer.Write(aBuffer.GetPointer(), aBuffer.GetSize()) != aBuffer.GetSize())
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