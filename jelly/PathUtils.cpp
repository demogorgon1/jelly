#include <sstream>
#include <vector>

#include <jelly/ErrorUtils.h>

#include "PathUtils.h"

namespace jelly
{

	namespace PathUtils
	{

		std::string
		MakePath(
			const char*						aRoot,
			FileType						aFileType,
			uint32_t						aNodeId,
			uint32_t						aId)
		{
			const char* typeString = (aFileType == FILE_TYPE_WAL) ? "wal" : "store";
			char path[1024];
			size_t result = (size_t)std::snprintf(path, sizeof(path), "%s/jelly-%s-%u-%u.bin", aRoot, typeString, aNodeId, aId);
			JELLY_CHECK(result <= sizeof(path), "Path too long.");
			return path;
		}

		bool
		ParsePath(
			const std::filesystem::path&	aPath,
			FileType&						aOutFileType,
			uint32_t&						aOutNodeId,
			uint32_t&						aOutId)
		{
			try
			{
				if (aPath.extension() != ".bin")
					return false;

				std::stringstream tokenizer(aPath.filename().replace_extension("").string());

				std::string token;
				std::vector<std::string> tokens;
				while (std::getline(tokenizer, token, '-'))
					tokens.push_back(token);

				if (tokens.size() != 4)
					return false;

				if (tokens[0] != "jelly")
					return false;

				if (tokens[1] == "store")
					aOutFileType = FILE_TYPE_STORE;
				else if (tokens[1] == "wal")
					aOutFileType = FILE_TYPE_WAL;
				else
					return false;

				aOutNodeId = (uint32_t)std::stoul(tokens[2]);
				aOutId = (uint32_t)std::stoul(tokens[3]);
			}
			catch (...)
			{
				return false;
			}

			return true;
		}

	}

}