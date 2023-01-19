#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>

#include "ThreadIndex.h"

namespace jelly
{

	namespace 
	{
		std::atomic_uint32_t			g_nextThreadIndex = 0;
		JELLY_THREAD_LOCAL(uint32_t)	g_threadIndex = UINT32_MAX;
	}

	namespace ThreadIndex
	{

		uint32_t	
		Get()
		{
			if(g_threadIndex == UINT32_MAX)
				g_threadIndex = g_nextThreadIndex++;

			JELLY_ASSERT(g_threadIndex < MAX_THREADS);

			return g_threadIndex;
		}

	}

}