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
				const Config* aConfig)
			{			
				typedef BlobNode<UIntKey<uint32_t>, Blob<1>, UIntKey<uint32_t>::Hasher> BlobNodeType;

				#if defined(JELLY_ZSTD)
					DefaultHost host(".", "wrtest", DefaultHost::COMPRESSION_MODE_ZSTD);
				#else
					DefaultHost host(".", "wrtest", DefaultHost::COMPRESSION_MODE_NONE);
				#endif

				BlobNodeType::Config config;					

				if(aConfig->m_readTestBlobCountMemoryLimit != UINT32_MAX)
					config.m_maxResidentBlobCount = aConfig->m_readTestBlobCountMemoryLimit;

				PerfTimer restartTimer;
				BlobNodeType blobNode(&host, 0, config);
				printf("Restarted node in %u ms...\n", (uint32_t)restartTimer.GetElapsedMilliseconds());

				if(aConfig->m_readTestBlobCount > 0)
				{
					std::vector<std::unique_ptr<BlobNodeType::Request>> requests;
					requests.resize((size_t)aConfig->m_readTestBlobCount);
					
					{
						PerfTimer t;

						for (uint32_t i = 0; i < aConfig->m_readTestBlobCount; i++)
						{
							requests[i] = std::make_unique<BlobNodeType::Request>();
							BlobNodeType::Request* req = requests[i].get();

							req->m_key = i;

							blobNode.Get(req);
						}

						printf("Queued up read requests in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}

					{
						PerfTimer t;

						blobNode.ProcessRequests();

						printf("Processed read requests in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}

					{
						PerfTimer t;

						std::mt19937 random(12345);

						for(std::unique_ptr<BlobNodeType::Request>& req : requests)
						{
							JELLY_ASSERT(req->m_completed.Poll());
							JELLY_ASSERT(req->m_result == RESULT_OK);

							const uint8_t* p = (const uint8_t*)req->m_blob.GetBuffer().GetPointer();
							size_t size = req->m_blob.GetBuffer().GetSize();
							for(size_t i = 0; i < size; i++)
							{
								uint8_t expectedByte = (uint8_t)(random() % 64);
								JELLY_ASSERT(p[i] == expectedByte);
							}
						}

						printf("Verified read requests in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}
				}
			}

		}

	}

}