#if defined(_WIN32)
	#include <windows.h>
#endif

#include <stdlib.h>

#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	namespace ErrorUtils
	{

		void
		Terminate(
			const char*		aFormat,
			...)
		{
			// FIXME: some kind of logging hook
			char buffer[2048];
			JELLY_STRING_FORMAT_VARARGS(buffer, sizeof(buffer), aFormat);
			Log::Print(Log::LEVEL_ERROR, buffer);

			#if defined(_WIN32)
				if(IsDebuggerPresent())
					DebugBreak();
			#endif

			exit(EXIT_FAILURE);
		}			

		void			
		AssertFailed(
			const char*		aFile,
			int				aLineNum,
			const char*		aAssertString,
			const char*		aMessageFormat,
			...)
		{
			fprintf(stderr, "ASSERT FAILED: %s:%d: %s\n", aFile, aLineNum, aAssertString);

			char buffer[2048];
			JELLY_STRING_FORMAT_VARARGS(buffer, sizeof(buffer), aMessageFormat);
			if(buffer[0] != '\0')
				fprintf(stderr, "%s\n", buffer);

			#if defined(_DEBUG)
				DebugBreak();
			#else
				exit(EXIT_FAILURE);
			#endif
		}

		void
		DebugBreak()
		{
			#if defined(_WIN32)
				__debugbreak();
			#else
				raise(SIGINT);
			#endif
		}
	}

}