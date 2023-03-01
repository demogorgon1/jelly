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
			JELLY_CHECK(value <= UINT32_MAX, Result::ERROR_INVALID_UINT32, "String=%s", aString);
			return (uint32_t)value;
		}
		
		size_t		
		ParseSize(
			const char* aString)
		{
			char* suffix = NULL;
			uintmax_t value = strtoumax(aString, &suffix, 10);
			JELLY_CHECK(value <= SIZE_MAX, Result::ERROR_INVALID_SIZE, "String=%s", aString);

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
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						JELLY_CHECK((value & ((1024ULL - 1ULL) << (SIZE_BITS - 10ULL))) == 0, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						multiplier = 1024ULL;
						break;

					case 'M':
					case 'm':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 10ULL))) == 0, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						multiplier = 1024ULL * 1024ULL;
						break;

					case 'G':
					case 'g':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 30ULL))) == 0, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						multiplier = 1024ULL * 1024ULL * 1024ULL;
						break;

					case 'T':
					case 't':
						JELLY_CHECK(!multiplier.has_value() || hasByteTag, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						JELLY_CHECK((value & (((1024ULL * 1024ULL * 1024ULL * 1024ULL) - 1ULL) << (SIZE_BITS - 40ULL))) == 0, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
						break;

					case 'B':
					case 'b':
						JELLY_CHECK(!hasByteTag, Result::ERROR_INVALID_SIZE, "String=%s", aString);
						hasByteTag = true;
						break;

					case ' ':
					case '\t':
						break;

					default:
						JELLY_FAIL(Result::ERROR_INVALID_SIZE, "String=%s", aString);
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
			if (strcmp(aString, "true") == 0 || strcmp(aString, "1") == 0)
				return true;
			if (strcmp(aString, "false") == 0 || strcmp(aString, "0") == 0)
				return false;

			JELLY_FAIL(Result::ERROR_INVALID_BOOL, "String=%s", aString);
			return false;
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
				JELLY_CHECK(value <= UINT32_MAX, Result::ERROR_INVALID_INTERVAL, "String=%s", aString);
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

			JELLY_CHECK(v <= UINT32_MAX, Result::ERROR_INTERVAL_OUT_OF_BOUNDS, "String=%s", aString);

			return (uint32_t)v;
		}

		std::string 
		MakeSizeString(
			uint64_t		aValue)
		{
			char buffer[64];
			double v = (double)aValue;
			int result = 0;
			if (v > 1024.0f * 1024.0f * 1024.0f)
				result = snprintf(buffer, sizeof(buffer), "%.1fG", v / (1024.0f * 1024.0f * 1024.0f));
			else if (aValue > 1024.0f * 1024.0f)
				result = snprintf(buffer, sizeof(buffer), "%.1fM", v / (1024.0f * 1024.0f));
			else if (aValue > 1024.0f)
				result = snprintf(buffer, sizeof(buffer), "%.1fK", v / (1024.0f));
			else
				result = snprintf(buffer, sizeof(buffer), "%.0f", v);
			JELLY_CHECK((size_t)result <= sizeof(buffer), Result::ERROR_SIZE_STRING_TOO_LONG, "Size=%zu", aValue);
			return buffer;
		}

		std::string	
		GetFileNameFromPath(
			const char*		aPath)
		{
			size_t strLen = strlen(aPath);
			JELLY_CHECK(strLen > 0, Result::ERROR_PATH_IS_EMPTY);

			for(size_t i = 0; i < strLen; i++)
			{
				char c = aPath[strLen - i - 1];
				if(c == '/' || c == '\\')
					return &aPath[strLen - i];
			}

			return aPath;
		}

	}

}