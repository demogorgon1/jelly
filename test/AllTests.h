#pragma once

namespace jelly
{

	namespace Test
	{

		struct Config;

		namespace AllTests
		{

			void		Run(
							const char*		aWorkingDirectory,
							const Config*	aConfig);

		}

	}

}