#include <stdlib.h>

#include "AllTests.h"
#include "Config.h"

int
main(
	int		aNumArgs,
	char**	aArgs)
{	
	jelly::Test::Config config;
	config.InitFromCommandLine(aNumArgs, aArgs);

	config.m_simTest = true;

	jelly::Test::AllTests::Run(".", &config);

	return EXIT_SUCCESS;
}