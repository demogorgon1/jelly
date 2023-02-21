#pragma once

#if defined(_WIN32)

	#define JELLY_STRING_FORMAT_VARARGS(Buffer, BufferSize, Format)						\
		{																				\
			va_list _list;																\
			va_start(_list, Format);													\
			int n = vsnprintf_s(Buffer, BufferSize, _TRUNCATE, Format, _list);			\
			if(n < 0)																	\
				Buffer[0] = '\0';														\
			va_end(_list);																\
		}

#else

	#define JELLY_STRING_FORMAT_VARARGS(Buffer, BufferSize, Format)						\
		{																				\
			va_list _list;																\
			va_start(_list, Format);													\
			int n = vsnprintf(Buffer, BufferSize, Format, _list);						\
			if(n < 0)																	\
				Buffer[0] = '\0';														\
			va_end(_list);																\
		}

#endif

namespace jelly
{

	namespace StringUtils
	{

		std::string	Format(
						const char*		aFormat,
						...);
		uint32_t	ParseUInt32(
						const char*		aString);
		size_t		ParseSize(
						const char*		aString);
		bool		ParseBool(
						const char*		aString);
		uint32_t	ParseInterval(
						const char*		aString);
		std::string MakeSizeString(
						uint64_t		aValue);
		std::string	GetFileNameFromPath(
						const char*		aPath);

	}

}