#pragma once

#include <jelly/ErrorUtils.h>
#include <jelly/IReader.h>
#include <jelly/IWriter.h>

namespace jelly
{

	namespace Test
	{

		struct UInt32Blob
		{
			UInt32Blob(uint32_t aValue = 0) : m_value(aValue), m_loaded(aValue != 0) {}

			bool		
			Write(
				IWriter*						aWriter,
				const Compression::IProvider*	/*aCompression*/) const
			{ 
				JELLY_ASSERT(m_loaded); 
					
				if(aWriter->Write(&m_value, sizeof(m_value)) != sizeof(m_value))
					return false;

				uint32_t fluff[10];
				for (uint32_t i = 0; i < 10; i++)
					fluff[i] = m_value * i;

				if(aWriter->Write(fluff, sizeof(fluff)) != sizeof(fluff))
					return false;

				return true;
			}
				
			bool		
			Read(
				IReader*						aReader,
				const Compression::IProvider*	/*aCompression*/) 
			{ 
				m_loaded = true;

				if(aReader->Read(&m_value, sizeof(m_value)) != sizeof(m_value))
					return false;

				uint32_t fluff[10];
				if (aReader->Read(fluff, sizeof(fluff)) != sizeof(fluff))
					return false;

				for (uint32_t i = 0; i < 10; i++)
					JELLY_ASSERT(fluff[i] == m_value * i);

				return true;
			}

			bool		operator==(const UInt32Blob& aOther) const { JELLY_ASSERT(m_loaded && aOther.m_loaded); return m_value == aOther.m_value; }				
			bool		IsSet() const { return m_loaded; }
			void		Reset() { JELLY_ASSERT(m_loaded); m_value = 0; m_loaded = false; }
			size_t		GetSize() const { return m_value == UINT32_MAX ? 0 : sizeof(uint32_t) + 10 * sizeof(uint32_t); }
			size_t		GetStoredSize() const { return GetSize(); }
			void		Move(UInt32Blob& aOther) { *this = aOther; }
			void		Copy(const UInt32Blob& aOther) { *this = aOther; }
			void		Delete() { JELLY_ASSERT(m_loaded); m_value = UINT32_MAX; }

			uint32_t	m_value;
			bool		m_loaded;
		};

	}

}