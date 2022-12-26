#include "CompressionTest.h"
#include "NodeTest.h"

namespace jelly
{

	namespace Test
	{
		
		namespace AllTests
		{

			void		
			Run(
				const char*		aWorkingDirectory)
			{
				CompressionTest::Run();
				NodeTest::Run(aWorkingDirectory);
			}

		}

	}

}