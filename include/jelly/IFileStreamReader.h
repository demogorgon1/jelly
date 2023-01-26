#pragma once

#include "IReader.h"

namespace jelly
{

	// Interface for file stream reader implementations
	class IFileStreamReader
		 : public IReader
	{
	public:
		virtual			~IFileStreamReader() {}

		// Virtual interface
		virtual bool	IsEnd() const = 0;
	};

}