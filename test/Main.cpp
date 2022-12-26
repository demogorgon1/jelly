#include <stdlib.h>

#include "AllTests.h"

int
main(
	int		/*aNumArgs*/,
	char**	/*aArgs*/)
{	
	jelly::Test::AllTests::Run(".");

	return EXIT_SUCCESS;
}