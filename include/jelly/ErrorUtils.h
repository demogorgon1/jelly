#pragma once

#include "Exception.h"
#include "Log.h"
#include "StringUtils.h"

#if defined(JELLY_SIMULATE_ERRORS)
	#define JELLY_SIMULATE_ERROR(_Error)									\
		if(jelly::ErrorUtils::ShouldSimulateError(_Error))					\
		{																	\
			jelly::ErrorUtils::CheckFailed(_Error, NULL);					\
			break;															\
		}
#else
	#define JELLY_SIMULATE_ERROR(_Error)
#endif

#define JELLY_CHECK(_Condition, _Error, ...)								\
	do																		\
	{																		\
		JELLY_SIMULATE_ERROR(_Error);										\
		if(!(_Condition))													\
			jelly::ErrorUtils::CheckFailed(_Error, "" __VA_ARGS__);			\
	} while(false)

#define JELLY_ALWAYS_ASSERT(_Condition, ...)								\
	do																		\
	{																		\
		if(!(_Condition))													\
			jelly::ErrorUtils::AssertFailed(								\
				__FILE__,													\
				__LINE__,													\
				#_Condition, 												\
				"" __VA_ARGS__);											\
	} while(false)

#define JELLY_FAIL(_Error, ...)												\
	jelly::ErrorUtils::CheckFailed(_Error, "" __VA_ARGS__)	

#if !defined(NDEBUG)
	#define JELLY_ASSERT JELLY_ALWAYS_ASSERT
#else
	#define JELLY_ASSERT(...) do { } while(false)
#endif

#if defined(JELLY_EXTRA_ERROR_INFO)
	#define JELLY_CONTEXT(_Context) ErrorUtils::ScopedContext scopedContext(_Context)
	#define JELLY_REQUEST_TYPE(_RequestType) ErrorUtils::ScopedRequestType scopedRequestType(_RequestType)
#else
	#define JELLY_CONTEXT(_Context)
	#define JELLY_REQUEST_TYPE(_RequestType)
#endif

namespace jelly
{

	namespace ErrorUtils
	{

#if defined(JELLY_EXTRA_ERROR_INFO)
		extern JELLY_THREAD_LOCAL(uint32_t)	g_threadCurrentContext;
		extern JELLY_THREAD_LOCAL(uint32_t)	g_threadCurrentRequestType;
#endif

		//----------------------------------------------------------------------------------------

		void			Terminate(
							const char*													aFormat,
							...);
		void			AssertFailed(
							const char*													aFile,
							int															aLineNum,
							const char*													aAssertString,
							const char*													aMessageFormat,
							...);
		void			CheckFailed(
							Exception::Error											aError,
							const char*													aMessageFormat,
							...);
		void			DebugBreak();

#if defined(JELLY_SIMULATE_ERRORS)
		void			ResetErrorSimulation();
		void			SimulateError(
							Exception::Error											aError,
							uint32_t													aProbability,
							uint32_t													aOccurances);						
		bool			ShouldSimulateError(	
							Exception::Error											aError);
#endif

		//----------------------------------------------------------------------------------------

#if defined(JELLY_EXTRA_ERROR_INFO)
		inline bool			
		EnterContext(
			uint32_t																	aContext) noexcept
		{
			if(g_threadCurrentContext == Exception::CONTEXT_NONE)
			{
				g_threadCurrentContext = aContext;
				return true;
			}
			
			return false;
		}
		
		inline uint32_t
		GetContext() noexcept
		{
			return g_threadCurrentContext;
		}

		inline void
		LeaveContext(
			uint32_t																	aContext) noexcept
		{
			JELLY_ASSERT(g_threadCurrentContext == aContext);
			JELLY_UNUSED(aContext);
			g_threadCurrentContext = Exception::CONTEXT_NONE;
		}
		
		inline void
		EnterRequestType(
			uint32_t																	aRequestType) noexcept
		{
			JELLY_ASSERT(g_threadCurrentRequestType == Exception::REQUEST_TYPE_NONE);
			g_threadCurrentRequestType = aRequestType;
		}

		inline uint32_t
		GetRequestType() noexcept
		{
			return g_threadCurrentRequestType;
		}
		
		inline void
		LeaveRequestType(
			uint32_t																	aRequestType) noexcept
		{
			JELLY_ASSERT(g_threadCurrentRequestType == aRequestType);
			JELLY_UNUSED(aRequestType);
			g_threadCurrentRequestType = Exception::REQUEST_TYPE_NONE;
		}

		//----------------------------------------------------------------------------------------

		struct ScopedContext
		{
			ScopedContext(
				uint32_t																aContext) noexcept
			{
				JELLY_ASSERT(aContext != Exception::CONTEXT_NONE);

				if(EnterContext(aContext))
					m_context = aContext;
				else
					m_context = Exception::CONTEXT_NONE;
			}

			~ScopedContext()
			{
				if(m_context != Exception::CONTEXT_NONE)
					LeaveContext(m_context);
			}

			uint32_t		m_context;
		};

		struct ScopedRequestType
		{
			ScopedRequestType(
				uint32_t																aRequestType) noexcept
				: m_requestType(aRequestType)
			{
				EnterRequestType(m_requestType);
			}

			~ScopedRequestType()
			{
				LeaveRequestType(m_requestType);
			}

			uint32_t		m_requestType;
		};

#endif /* JELLY_EXTRA_ERROR_INFO */

	}

}