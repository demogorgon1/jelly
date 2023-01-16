#pragma once

namespace jelly
{

	namespace Test
	{

		struct UInt32Blob
		{
			UInt32Blob(uint32_t aValue = 0) : m_value(aValue), m_loaded(aValue != 0) {}

			void
			ToBuffer(
				const Compression::IProvider*	/*aCompression*/,	
				IBuffer&						aOut) const
			{
				JELLY_ASSERT(m_loaded);
				JELLY_ASSERT(aOut.GetSize() == 0);
				BufferWriter writer(aOut);

				if(!writer.WriteUInt<uint32_t>(m_value))
					JELLY_ASSERT(false);

				// Add some fluff to make the blob a bit bigger
				uint32_t fluff[10];
				for (uint32_t i = 0; i < 10; i++)
					fluff[i] = m_value * i;

				if(writer.Write(fluff, sizeof(fluff)) != sizeof(fluff))
					JELLY_ASSERT(false);
			}

			void
			FromBuffer(
				const Compression::IProvider*	/*aCompression*/,
				const IBuffer&					aBuffer) 
			{
				BufferReader reader(aBuffer.GetPointer(), aBuffer.GetSize());

				m_loaded = true;

				if(!reader.ReadUInt<uint32_t>(m_value))
					JELLY_ASSERT(false);

				uint32_t fluff[10];
				if (reader.Read(fluff, sizeof(fluff)) != sizeof(fluff))
					JELLY_ASSERT(false);

				for (uint32_t i = 0; i < 10; i++)
					JELLY_ASSERT(fluff[i] == m_value * i);
			}

			bool		
			operator==(
				const UInt32Blob&				aOther) const 
			{
				JELLY_ASSERT(m_loaded && aOther.m_loaded); 
				return m_value == aOther.m_value;
			}

			bool
			operator==(
				uint32_t						aOther) const
			{
				JELLY_ASSERT(m_loaded);
				return m_value == aOther;
			}

			// Public data
			uint32_t	m_value;
			bool		m_loaded;
		};

	}

}