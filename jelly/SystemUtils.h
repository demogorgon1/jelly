#pragma once

namespace jelly::SystemUtils
{

	struct MemoryInfo
	{
		size_t		m_usage = 0;
	};

	MemoryInfo			GetMemoryInfo();

}