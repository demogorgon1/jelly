#pragma once

// This header file must be included before any other jelly headers.

#if !defined(_WIN32)
	#include <signal.h>
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define JELLY_UNUSED(_X) ((void)_X)

#if defined(_MSC_VER)
	#define JELLY_THREAD_LOCAL(_Type) __declspec(thread) _Type
#else
	#define JELLY_THREAD_LOCAL(_Type) thread_local _Type
#endif

#if defined(JELLY_SIMULATE_ERRORS) && !defined(_DEBUG)
	// We'll only support simulating errors in debug builds
	#undef JELLY_SIMULATE_ERRORS
#endif

#if defined(JELLY_SIMULATE_ERRORS) && !defined(JELLY_EXTRA_ERROR_INFO)
	// We need extra error information enabled to support simulating errors
	#undef JELLY_SIMULATE_ERRORS
#endif