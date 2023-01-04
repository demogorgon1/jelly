#pragma once

#include <stdint.h>

namespace jelly
{

	namespace Test
	{

		struct Config;

		namespace NodeTest
		{

			void		Run(
							const char*		aWorkingDirectory,
							const Config*	aConfig);

		}

	}

}