#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	namespace StringUtils
	{		

		std::string	
		Format(
			const char* aFormat,
			...)
		{
			char buffer[1024];
			JELLY_STRING_FORMAT_VARARGS(buffer, sizeof(buffer), aFormat);
			return buffer;
		}

		uint32_t	
		ParseUInt32(
			const char* aString)
		{
			uintmax_t value = strtoumax(aString, NULL, 10);
			JELLY_CHECK(value <= UINT32_MAX, "Invalid uint32: %s", aString);
			return (uint32_t)value;
		}
		
		size_t		
		ParseSize(
			const char* aString)
		{
			char* suffix = NULL;
			uintmax_t value = strtoumax(aString, &suffix, 10);
			JELLY_CHECK(value <= SIZE_MAX, "Invalid size: %s", aString);

			if(suffix != NULL)
			{
				std::optional<uintmax_t> multiplier; 
				static const uintmax_t SIZE_BITS = sizeof(size_t) * 8;
				bool hasByteTag = false;

				while(*suffix != '\0')
				{
					switch(*suffix)
					{
					case 'K':
					case 'k':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, "Invalid size: %s", aString);
						JELLY_CHECK((value & ((1024ULL - 1ULL) << (SIZE_BITS - 10ULL))) == 0, "Size exceeds %u bits: %s", (uint32_t)SIZE_BITS, aString);
						multiplier = 1024ULL;
						break;

					case 'M':
					case 'm':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, "Invalid size: %s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 10ULL))) == 0, "Size exceeds %u bits: %s", (uint32_t)SIZE_BITS, aString);
						multiplier = 1024ULL * 1024ULL;
						break;

					case 'G':
					case 'g':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, "Invalid size: %s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 30ULL))) == 0, "Size exceeds %u bits: %s", (uint32_t)SIZE_BITS, aString);
						multiplier = 1024ULL * 1024ULL * 1024ULL;
						break;

					case 'T':
					case 't':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, "Invalid size: %s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL * 1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 40ULL))) == 0, "Size exceeds %u bits: %s", (uint32_t)SIZE_BITS, aString);
						multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
						break;

					case 'B':
					case 'b':
						JELLY_CHECK(!hasByteTag, "Invalid size: %s", aString);
						hasByteTag = true;
						break;

					case ' ':
					case '\t':
						break;

					default:
						JELLY_FATAL_ERROR("Invalid size: %s", aString);
					}

					suffix++;
				}			
				
				if(multiplier.has_value())
					value *= multiplier.value();
			}

			return (size_t)value;
		}
		
		bool		
		ParseBool(
			const char* aString)
		{
			size_t len = strlen(aString);
			JELLY_CHECK(len == 1, "Invalid bool: %s", aString);

			bool value = false;

			if(aString[0] == '0')
				value = false;
			else if (aString[0] == '1')
				value = true;
			else 
				JELLY_FATAL_ERROR("Invalid bool: %s", aString);
			
			return value;
		}

		uint32_t	
		ParseInterval(
			const char* aString)
		{
			const char* p = aString;

			uintmax_t v = 0;

			while(p != NULL && *p != '\0')
			{
				char* next = NULL;
				uintmax_t value = strtoumax(p, &next, 10);
				JELLY_CHECK(value <= UINT32_MAX, "Invalid interval: %s", aString);
				p = next;

				if(p != NULL && *p != '\0')
				{
					if ((p[0] == 'm' || p[0] == 'M') && (p[1] == 's' || p[1] == 'S'))
					{
						p += 2;
					}
					else if (p[0] == 's' || p[0] == 'S') 
					{
						value *= 1000;
						p++;
					}
					else if (p[0] == 'm' || p[0] == 'M')
					{
						value *= 1000 * 60;
						p++;
					}
					else if (p[0] == 'h' || p[0] == 'H')
					{
						value *= 1000 * 60 * 60;
						p++;
					}
					else if (p[0] == 'd' || p[0] == 'D')
					{
						value *= 1000 * 60 * 60 * 24;
						p++;
					}
				}

				v += value;
			}

			JELLY_CHECK(v <= UINT32_MAX, "Interval out of bounds: %s", aString);

			return (uint32_t)v;
		}


	}

}