#pragma once

#define JELLY_CHECK(_Condition, ...)										\
	do																		\
	{																		\
		if(!(_Condition))													\
			jelly::ErrorUtils::Terminate("" __VA_ARGS__);					\
	} while(false)

#define JELLY_ASSERT(_Condition, ...)										\
	do																		\
	{																		\
		if(!(_Condition))													\
			jelly::ErrorUtils::AssertFailed(								\
				__FILE__,													\
				__LINE__,													\
				#_Condition, 												\
				"" __VA_ARGS__);											\
	} while(false)
	
#define JELLY_FATAL_ERROR(...)												\
	jelly::ErrorUtils::Terminate("" __VA_ARGS__)

namespace jelly
{

	namespace ErrorUtils
	{

		void			Terminate(
							const char*			aFormat,
							...);
		void			AssertFailed(
							const char*			aFile,
							int					aLineNum,
							const char*			aAssertString,
							const char*			aMessageFormat,
							...);
		void			DebugBreak();

	}

}