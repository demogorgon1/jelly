#include <jelly/API.h>

#include <jelly/StringUtils.h>

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
				typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;

				DefaultHost host(".", "wrtest", NULL);

				if(aConfig->m_readTestBlobCountMemoryLimit != UINT32_MAX)
					host.GetDefaultConfigSource()->Set(jelly::Config::ID_MAX_RESIDENT_BLOB_SIZE, StringUtils::Format("%u", aConfig->m_readTestBlobCountMemoryLimit).c_str());

				PerfTimer restartTimer;
				BlobNodeType blobNode(&host, 0);
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

							req->SetKey(i);

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
							JELLY_ASSERT(req->IsCompleted());
							JELLY_ASSERT(req->GetResult() == RESULT_OK);

							const uint8_t* p = (const uint8_t*)req->GetBlob()->GetPointer();
							size_t size = req->GetBlob()->GetSize();
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