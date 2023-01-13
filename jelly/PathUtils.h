#pragma once

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
						const char*						aFilePrefix,
						FileType						aFileType,
						uint32_t						aNodeId,
						uint32_t						aId);
		bool		ParsePath(
						const std::filesystem::path&	aPath,
						const char*						aFilePrefix,
						FileType&						aOutFileType,
						uint32_t&						aOutNodeId,
						uint32_t&						aOutId);

	}

}