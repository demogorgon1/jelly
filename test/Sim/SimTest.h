#pragma once 

namespace jelly::Test
{

	struct Config;

	namespace Sim
	{

		void		SimTest(
						const char*		aWorkingDirectory,
						const Config*	aConfig);

	}

}