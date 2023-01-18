#include <jelly/API.h>

#include "Config.h"

namespace jelly
{

	namespace Test
	{

		namespace WriteTest
		{

			void		
			Run(
				const Config* aConfig)
			{			
				std::vector<Blob<1>> blobs;

				// Create blobs
				{
					PerfTimer t;

					blobs.resize((size_t)aConfig->m_writeTestBlobCount);

					std::mt19937 random(12345);

					for (Blob<1>& blob : blobs)
					{
						size_t blobSize = (size_t)aConfig->m_writeTestBlobSize;
						blob.GetBuffer().SetSize(blobSize);

						for (size_t j = 0; j < blobSize; j++)
							((uint8_t*)blob.GetBuffer().GetPointer())[j] = (uint8_t)(random() % 64); // (not COMPLETELY random, stay in 0-63 range for a bit of compression)
					}

					printf("Created blobs in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
				}

				{
					typedef BlobNode<UIntKey<uint32_t>, Blob<1>, UIntKey<uint32_t>::Hasher> BlobNodeType;

					DefaultHost host(".", "wrtest", DefaultHost::COMPRESSION_MODE_ZSTD);

					host.DeleteAllFiles(UINT32_MAX);

					BlobNodeType::Config config;					
					BlobNodeType blobNode(&host, 0, config);

					// Queue up all requests
					std::vector<std::unique_ptr<BlobNodeType::Request>> requests;
					requests.resize(blobs.size());

					{
						PerfTimer t;

						for (size_t i = 0; i < blobs.size(); i++)
						{
							requests[i] = std::make_unique<BlobNodeType::Request>();
							BlobNodeType::Request* req = requests[i].get();

							req->m_key = (uint32_t)i;
							req->m_seq = 0;
							req->m_blob = blobs[i];

							blobNode.Set(req);
						}

						printf("Queued up write requests in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}

					// Process requests
					{
						PerfTimer t;

						blobNode.ProcessRequests();

						printf("Processed write requests in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}

					// Flush pending WAL
					{
						PerfTimer t;

						blobNode.FlushPendingWAL(0);

						printf("Flushed pending WAL in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}

					// Flush pending store
					{
						PerfTimer t;

						blobNode.FlushPendingStore();

						printf("Flushed pending store in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
					}					
				}
			}

		}

	}

}