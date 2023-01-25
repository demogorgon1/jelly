#pragma once

namespace jelly
{

	// DefaultHost: utility for dealing with WAL and store file names.
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