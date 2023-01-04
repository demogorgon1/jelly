#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	namespace Log
	{

		Callback g_callback;

		const char* 
		LevelToString(
			Level			aLevel)
		{
			switch(aLevel)
			{
			case LEVEL_INFO: return "INFO";
			case LEVEL_WARNING: return "WARNING";
			case LEVEL_ERROR: return "ERROR";
			}
			JELLY_ASSERT(false);
			return "";
		}

		void	
		SetCallback(
			Callback		aCallback)
		{
			g_callback = aCallback;
		}

		void	
		Print(
			Level			aLevel,
			const char*		aMessage)
		{
			if(g_callback)
				g_callback(aLevel, aMessage);
			else
				fprintf(aLevel == LEVEL_ERROR ? stderr : stdout, "[%s] %s\n", LevelToString(aLevel), aMessage);
		}
		
		void	
		PrintF(
			Level			aLevel,
			const char*		aFormat,
			...)
		{
			char buffer[2048];
			JELLY_STRING_FORMAT_VARARGS(buffer, sizeof(buffer), aFormat);
			Print(aLevel, aFormat);
		}

	}

}