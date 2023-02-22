#include <jelly/API.h>

#include "AllTests.h"
#include "Config.h"

int
main(
	int		aNumArgs,
	char**	aArgs)
{	
	jelly::Test::Config config;
	config.InitFromCommandLine(aNumArgs, aArgs);

	jelly::Test::AllTests::Run(".", &config);

	return EXIT_SUCCESS;
}