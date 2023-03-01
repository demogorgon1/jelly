#if defined(_WIN32)
	#include <windows.h>

	#define getpid _getpid
#endif

#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	namespace ErrorUtils
	{

		ProcessFingerprint::ProcessFingerprint()
		{		
			m_fingerprint = (uint32_t)time(NULL);
			m_fingerprint ^= std::hash<std::thread::id>{}(std::this_thread::get_id());
			m_fingerprint ^= (uint32_t)getpid();			
		}

		ProcessFingerprint	g_processFingerprint;

		//----------------------------------------------------------------------------------------

		JELLY_THREAD_LOCAL(uint32_t) g_threadCurrentContext = Result::CONTEXT_NONE;
		JELLY_THREAD_LOCAL(uint32_t) g_threadCurrentRequestType = Result::REQUEST_TYPE_NONE;

		//----------------------------------------------------------------------------------------

		void
		Terminate(
			const char*		aFormat,
			...)
		{
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
				#if defined(_WIN32)
					if (IsDebuggerPresent())
						DebugBreak();
				#endif

				exit(EXIT_FAILURE);
			#endif
		}

		void			
		CheckFailed(
			Result::Error	aError,
			const char*		aMessageFormat,
			...)
		{
			uint32_t requestType = g_threadCurrentRequestType;
			uint32_t contextType = g_threadCurrentContext;

			// Make (somewhat) unique log finger print that can be baked into the result code. This makes it possible
			// to find logs associated with an result code returned to the application.
			static std::atomic_uint32_t counter = 0;			
			counter++;
			uint32_t logFingerprint = (uint32_t)std::hash<uint32_t>{}(counter ^ g_processFingerprint.m_fingerprint);
			logFingerprint &= Result::LOG_FINGERPRINT_BIT_MASK;

			// Log message
			JELLY_ASSERT(aError < Result::NUM_ERRORS);
			const Result::ErrorInfo* info = Result::GetErrorInfo(aError);
			const char* categoryString = Result::GetCategoryString(info->m_category);

			JELLY_ASSERT(requestType < Result::NUM_REQUEST_TYPES);
			const char* requestTypeString = Result::GetRequestTypeString(requestType);

			JELLY_ASSERT(contextType < Result::NUM_CONTEXTS);
			const char* contextString = Result::GetContextString(contextType);

			char messageBuffer[2048];
			JELLY_STRING_FORMAT_VARARGS(messageBuffer, sizeof(messageBuffer), aMessageFormat);

			Log::PrintF(Log::LEVEL_ERROR, "%s;CO=%s;CA=%s;R=%s;F=%08x;%s", 
				info->m_string,
				contextString,
				categoryString, 
				requestTypeString,
				logFingerprint,
				messageBuffer);

			// Assemble result code and throw it
			throw Result::MakeResultCode(aError, contextType, requestType, logFingerprint);
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