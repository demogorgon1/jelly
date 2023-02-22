#pragma once

namespace jelly
{

	namespace Test
	{
		
		struct UInt32Blob 
			: public Buffer<sizeof(uint32_t)>
		{
			UInt32Blob(
				uint32_t		aValue)
			{
				SetSize(sizeof(uint32_t));
				SetValue(aValue);
			}

			void
			SetValue(
				uint32_t		aValue)
			{
				JELLY_ALWAYS_ASSERT(GetSize() == sizeof(uint32_t));
				*((uint32_t*)GetPointer()) = aValue;
			}

			static uint32_t
			GetValue(
				const IBuffer*	aBuffer) 
			{
				JELLY_ALWAYS_ASSERT(aBuffer->GetSize() == sizeof(uint32_t));
				return *((uint32_t*)aBuffer->GetPointer());
			}
		};

	}

}