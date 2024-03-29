#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>

#include "PathUtils.h"

namespace jelly
{

	namespace
	{

		void
		_ValidateFilePrefix(
			const char*						aFilePrefix)
		{
			bool hasInvalidCharacters = false;

			const char* p = aFilePrefix;
			while(*p != '\0' && !hasInvalidCharacters)
			{
				if(*p == '-')
					hasInvalidCharacters = true;
				p++;
			}

			JELLY_CHECK(!hasInvalidCharacters, Exception::ERROR_INVALID_CHARACTER_IN_FILE_PREFIX, "FilePrefix=%s", aFilePrefix);
		}

	}

	namespace PathUtils
	{

		std::string
		MakePath(
			const char*						aRoot,
			const char*						aFilePrefix,
			FileType						aFileType,
			uint32_t						aNodeId,
			uint32_t						aId)
		{
			_ValidateFilePrefix(aFilePrefix);
			const char* typeString = (aFileType == FILE_TYPE_WAL) ? "wal" : "store";
			char path[1024];
			size_t result = (size_t)std::snprintf(path, sizeof(path), "%s/%sjelly-%s-%u-%u.bin", aRoot, aFilePrefix, typeString, aNodeId, aId);
			JELLY_CHECK(result <= sizeof(path), Exception::ERROR_PATH_TOO_LONG);
			return path;
		}

		bool
		ParsePath(
			const std::filesystem::path&	aPath,
			const char*						aFilePrefix,
			FileType&						aOutFileType,
			uint32_t&						aOutNodeId,
			uint32_t&						aOutId)
		{
			try
			{
				if (aPath.extension() != ".bin")
					return false;

				_ValidateFilePrefix(aFilePrefix);

				std::stringstream tokenizer(aPath.filename().replace_extension("").string());

				std::string token;
				std::vector<std::string> tokens;
				while (std::getline(tokenizer, token, '-'))
					tokens.push_back(token);

				if (tokens.size() != 4)
					return false;

				if (tokens[0] != std::string(aFilePrefix) + std::string("jelly"))
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