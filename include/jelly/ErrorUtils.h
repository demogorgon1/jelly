#pragma once

#include "Exception.h"
#include "Log.h"
#include "StringUtils.h"

#define JELLY_CHECK(_Condition, _Error, ...)								\
	do																		\
	{																		\
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

#define JELLY_CONTEXT(_Context) ErrorUtils::ScopedContext scopedContext(_Context)
#define JELLY_REQUEST_TYPE(_RequestType) ErrorUtils::ScopedRequestType scopedRequestType(_RequestType)

namespace jelly
{

	namespace ErrorUtils
	{

		struct ProcessFingerprint
		{
			ProcessFingerprint();

			uint32_t						m_fingerprint;
		};

		extern ProcessFingerprint			g_processFingerprint;

		//----------------------------------------------------------------------------------------

		extern JELLY_THREAD_LOCAL(uint32_t)	g_threadCurrentContext;
		extern JELLY_THREAD_LOCAL(uint32_t)	g_threadCurrentRequestType;

		//----------------------------------------------------------------------------------------

		void			Terminate(
							const char*			aFormat,
							...);
		void			AssertFailed(
							const char*			aFile,
							int					aLineNum,
							const char*			aAssertString,
							const char*			aMessageFormat,
							...);
		void			CheckFailed(
							Exception::Error	aError,
							const char*			aMessageFormat,
							...);
		void			DebugBreak();

		//----------------------------------------------------------------------------------------

		inline void			
		EnterContext(
			uint32_t			aContext) noexcept
		{
			JELLY_ASSERT(g_threadCurrentContext == Exception::CONTEXT_NONE);
			g_threadCurrentContext = aContext;
		}
		
		inline uint32_t
		GetContext() noexcept
		{
			return g_threadCurrentContext;
		}

		inline void
		LeaveContext(
			uint32_t			aContext) noexcept
		{
			JELLY_ASSERT(g_threadCurrentContext == aContext);
			g_threadCurrentContext = Exception::CONTEXT_NONE;
		}
		
		inline void
		EnterRequestType(
			uint32_t			aRequestType) noexcept
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
			uint32_t			aRequestType) noexcept
		{
			JELLY_ASSERT(g_threadCurrentRequestType == aRequestType);
			g_threadCurrentRequestType = Exception::REQUEST_TYPE_NONE;
		}

		//----------------------------------------------------------------------------------------

		struct ScopedContext
		{
			ScopedContext(
				uint32_t		aContext)
				: m_context(aContext)
			{
				EnterContext(m_context);
			}

			~ScopedContext()
			{
				LeaveContext(m_context);
			}

			uint32_t		m_context;
		};

		struct ScopedRequestType
		{
			ScopedRequestType(
				uint32_t		aRequestType)
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

	}

}