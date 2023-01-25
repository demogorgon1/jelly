#pragma once

// This header file must be included before any other jelly headers.

#if !defined(_WIN32)
	#include <signal.h>
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
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