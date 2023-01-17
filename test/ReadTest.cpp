#include <jelly/API.h>

#include "Config.h"

namespace jelly
{

	namespace Test
	{

		namespace ReadTest
		{

			void		
			Run(
				const Config* /*aConfig*/)
			{			
				typedef BlobNode<UIntKey<uint32_t>, Blob<1>, UIntKey<uint32_t>::Hasher> BlobNodeType;

				DefaultHost host(".", "wrtest", DefaultHost::COMPRESSION_MODE_ZSTD);

				BlobNodeType::Config config;					

				PerfTimer restartTimer;
				BlobNodeType blobNode(&host, 0, config);
				printf("Restarted node in %u ms...\n", (uint32_t)restartTimer.GetElapsedMilliseconds());
			}

		}

	}

}