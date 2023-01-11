#include <jelly/API.h>

#include "Sim/SimTest.h"

#include "CompressionTest.h"
#include "Config.h"
#include "MiscTest.h"
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
				MiscTest::Run();
				CompressionTest::Run();
				NodeTest::Run(aWorkingDirectory, aConfig);
				StepTest::Run(aConfig);

				if(aConfig->m_simTest)
					Sim::SimTest(aWorkingDirectory, aConfig);
			}

		}

	}

}