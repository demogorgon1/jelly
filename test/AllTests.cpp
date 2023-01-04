#include "CompressionTest.h"
#include "NodeTest.h"
#include "StepTest.h"

namespace jelly
{

	namespace Test
	{
		
		namespace AllTests
		{

			void		
			Run(
				const char*		aWorkingDirectory,
				const Config*	aConfig)
			{
				CompressionTest::Run();
				NodeTest::Run(aWorkingDirectory, aConfig);
				StepTest::Run(aConfig);
			}

		}

	}

}