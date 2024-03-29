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
				std::vector<Blob> blobs;

				// Create blobs
				{
					PerfTimer t;

					blobs.resize((size_t)aConfig->m_writeTestBlobCount);

					std::mt19937 random(12345);

					for (Blob& blob : blobs)
					{
						size_t blobSize = (size_t)aConfig->m_writeTestBlobSize;
						blob.SetSize(blobSize);

						void* blobPointer = blob.GetPointer();

						for (size_t j = 0; j < blobSize; j++)
							((uint8_t*)blobPointer)[j] = (uint8_t)(random() % 64); // (not COMPLETELY random, stay in 0-63 range for a bit of compression)
					}

					printf("Created blobs in %u ms...\n", (uint32_t)t.GetElapsedMilliseconds());
				}

				{
					typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;

					DefaultHost host(".", "wrtest", NULL);

					host.DeleteAllFiles(UINT32_MAX);

					BlobNodeType blobNode(&host, 0);

					// Queue up all requests
					std::vector<std::unique_ptr<BlobNodeType::Request>> requests;
					requests.resize(blobs.size());

					{
						PerfTimer t;

						for (size_t i = 0; i < blobs.size(); i++)
						{
							requests[i] = std::make_unique<BlobNodeType::Request>();
							BlobNodeType::Request* req = requests[i].get();

							req->SetKey((uint32_t)i);
							req->SetSeq(0);
							req->SetBlob(blobs[i].Copy());

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