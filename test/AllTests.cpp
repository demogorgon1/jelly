#include <jelly/API.h>

#include "Sim/SimTest.h"

#include "AdvBlobTest.h"
#include "BackupTest.h"
#include "CompressionTest.h"
#include "Config.h"
#include "FileTest.h"
#include "GenerateDocs.h"
#include "HousekeepingAdvisorTest.h"
#include "ItemHashTableTest.h"
#include "MiscTest.h"
#include "NodeTest.h"
#include "ReadTest.h"
#include "ReplicationTest.h"
#include "StepTest.h"
#include "WriteTest.h"

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
				if(aConfig->m_generateDocs)
				{
					GenerateDocs::Run();
					return;
				}

				ItemHashTableTest::Run();
				FileTest::Run();
				CompressionTest::Run();
				MiscTest::Run();
				NodeTest::Run(aWorkingDirectory, aConfig);
				AdvBlobTest::Run();
				StepTest::Run(aConfig);
				HousekeepingAdvisorTest::Run();
				ReplicationTest::Run();
				BackupTest::Run();

				if(aConfig->m_simTest)
					Sim::SimTest(aWorkingDirectory, aConfig);

				if(aConfig->m_writeTest)
					WriteTest::Run(aConfig);

				if(aConfig->m_readTest)
					ReadTest::Run(aConfig);
			}

		}

	}

}