#pragma once

#include <stdint.h>

#include <filesystem>
#include <string>

namespace jelly
{

	namespace PathUtils
	{

		enum FileType
		{
			FILE_TYPE_WAL,
			FILE_TYPE_STORE
		};

		std::string	MakePath(
						const char*						aRoot,
						FileType						aFileType,
						uint32_t						aNodeId,
						uint32_t						aId);
		bool		ParsePath(
						const std::filesystem::path&	aPath,
						FileType&						aOutFileType,
						uint32_t&						aOutNodeId,
						uint32_t&						aOutId);

	}

}