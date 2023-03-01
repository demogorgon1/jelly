#pragma once

namespace jelly
{

	namespace ThreadIndex
	{
		
		static const uint32_t MAX_THREADS = 256;

		// Returns a unique id of the calling thread in the range of 0 to MAX_THREADS-1
		uint32_t	Get() noexcept;

	}

}