#include <jelly/Base.h>

#if defined(_WIN32)
	#include <windows.h>
	#include <psapi.h>	
#else
	#include <unistd.h>
#endif

#include <jelly/ErrorUtils.h>

#include "SystemUtils.h"

namespace jelly::SystemUtils
{

	MemoryInfo			
	GetMemoryInfo()
	{
		MemoryInfo mi;

		#if defined(_WIN32)			
			HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,	FALSE, GetCurrentProcessId());
			JELLY_CHECK(h != INVALID_HANDLE_VALUE, "OpenProcess() failed: %u", GetLastError());
			
			PROCESS_MEMORY_COUNTERS_EX t;			
			BOOL result = GetProcessMemoryInfo(h, (PPROCESS_MEMORY_COUNTERS)&t, (DWORD)sizeof(t));
			JELLY_CHECK(result != 0, "GetProcessMemoryInfo() failed: %u", GetLastError());			

			CloseHandle(h);

			mi.m_usage = (size_t)t.PrivateUsage;
		#else
			const char* path = "/proc/self/statm";
			FILE* fp = fopen(path, "r");
			JELLY_CHECK(fp != NULL, "fopen() failed: %u (path: %s)", errno, path);

			uint32_t rss;
			int result = fscanf(fp, "%*s%u", &rss);
			JELLY_CHECK(result == 1, "fscanf() failed: %u (path: %s)", errno, path);

			fclose(fp);

			mi.m_usage = (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
		#endif

		return mi;
	}

}