#if defined(_WIN32)
	#include <windows.h>

	#define getpid _getpid
#else
	#include <unistd.h>
#endif

#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	namespace ErrorUtils
	{

#if defined(JELLY_EXTRA_ERROR_INFO)
		struct ProcessFingerprint
		{
			ProcessFingerprint()
			{
				// Mix some kinda-but-no-so-much-random things together and get something that's kinda random, hopefully
				m_fingerprint = (uint32_t)time(NULL);
				m_fingerprint ^= std::hash<std::thread::id>{}(std::this_thread::get_id());
				m_fingerprint ^= (uint32_t)getpid();
			}

			// Public data
			uint32_t						m_fingerprint;
		};

		ProcessFingerprint	g_processFingerprint;
#endif

		//----------------------------------------------------------------------------------------

#if defined(JELLY_SIMULATE_ERRORS)
		struct ErrorSimulation
		{
			struct Entry
			{
				uint32_t					m_probability;
				uint32_t					m_occurances;
			};

			void
			AddError(
				Exception::Error											aError,
				uint32_t													aProbability,
				uint32_t													aOccurances)
			{
				m_errorProbabilities.insert(std::make_pair<uint32_t, Entry>((uint32_t)aError, { aProbability, aOccurances }));
			}

			bool 
			ShouldTrigger(
				Exception::Error											aError) 
			{
				std::unordered_map<uint32_t, Entry>::iterator it = m_errorProbabilities.find((uint32_t)aError);
				if(it == m_errorProbabilities.end())
					return false;

				if(it->second.m_probability == 0)
					return false;

				std::uniform_int_distribution<uint32_t> distribution(0, 99);

				std::lock_guard lock(m_randomLock);

				if(it->second.m_occurances == 0)
					return false;

				if(distribution(m_random) >= it->second.m_probability)
					return false;

				if(it->second.m_occurances != UINT32_MAX)
					it->second.m_occurances--;

				return true;
			}

			void
			Reset()
			{
				m_errorProbabilities.clear();
			}
			
			// Public data
			std::unordered_map<uint32_t, Entry>				m_errorProbabilities;

			std::mutex										m_randomLock;
			std::mt19937									m_random;
		};

		ErrorSimulation		g_errorSimulation;
#endif // JELLY_SIMULATE_ERRORS

		//----------------------------------------------------------------------------------------

#if defined(JELLY_EXTRA_ERROR_INFO)
		JELLY_THREAD_LOCAL(uint32_t) g_threadCurrentContext		= Exception::CONTEXT_NONE;
		JELLY_THREAD_LOCAL(uint32_t) g_threadCurrentRequestType = Exception::REQUEST_TYPE_NONE;
#endif

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
			Exception::Error	aError,
			const char*			aMessageFormat,
			...)
		{
			#if defined(JELLY_EXTRA_ERROR_INFO)
				uint32_t requestType = g_threadCurrentRequestType;
				uint32_t contextType = g_threadCurrentContext;

				// Make (somewhat) unique log finger print that can be baked into the result code. This makes it possible
				// to find logs associated with an result code returned to the application.
				static std::atomic_uint32_t counter = 0;			
				counter++;
				uint32_t logFingerprint = (uint32_t)std::hash<uint32_t>{}(counter ^ g_processFingerprint.m_fingerprint);
				logFingerprint &= Exception::LOG_FINGERPRINT_BIT_MASK;

				if (aMessageFormat != NULL)
				{
					// Log message
					JELLY_ASSERT(aError < Exception::NUM_ERRORS);
					const Exception::ErrorInfo* info = Exception::GetErrorInfo(aError);
					const char* categoryString = Exception::GetCategoryString(info->m_category);

					JELLY_ASSERT(requestType < Exception::NUM_REQUEST_TYPES);
					const char* requestTypeString = Exception::GetRequestTypeString(requestType);

					JELLY_ASSERT(contextType < Exception::NUM_CONTEXTS);
					const char* contextString = Exception::GetContextString(contextType);

					char messageBuffer[2048];
					JELLY_STRING_FORMAT_VARARGS(messageBuffer, sizeof(messageBuffer), aMessageFormat);

					Log::PrintF(Log::LEVEL_ERROR, "%s;CO=%s;CA=%s;R=%s;F=%08x;%s",
						info->m_string,
						contextString,
						categoryString,
						requestTypeString,
						logFingerprint,
						messageBuffer);
				}

				// Assemble exception code and throw it
				throw Exception::MakeExceptionCode(aError, contextType, requestType, logFingerprint);
			#else
				if(aMessageFormat != NULL)
				{
					JELLY_ASSERT(aError < Exception::NUM_ERRORS);
					const Exception::ErrorInfo* info = Exception::GetErrorInfo(aError);
					const char* categoryString = Exception::GetCategoryString(info->m_category);

					char messageBuffer[2048];
					JELLY_STRING_FORMAT_VARARGS(messageBuffer, sizeof(messageBuffer), aMessageFormat);

					Log::PrintF(Log::LEVEL_ERROR, "%s;CA=%s;%s",
						info->m_string,
						categoryString,
						messageBuffer);
				}

				// Assemble exception code and throw it
				throw Exception::MakeExceptionCode(aError, Exception::CONTEXT_NONE, Exception::REQUEST_TYPE_NONE, 0);
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

#if defined(JELLY_SIMULATE_ERRORS)
		void			
		ResetErrorSimulation()
		{
			g_errorSimulation.m_errorProbabilities.clear();
		}

		void			
		SimulateError(
			Exception::Error											aError,
			uint32_t													aProbability,
			uint32_t													aOccurances)
		{
			g_errorSimulation.AddError(aError, aProbability, aOccurances);
		}

		bool			
		ShouldSimulateError(
			Exception::Error											aError)
		{
			return g_errorSimulation.ShouldTrigger(aError);
		}
#endif

	}

}