/**
 * \example Example4_HousekeepingAdvisor.cpp
 * Using the housekeeping advisor to decide when housekeeping tasks should be done.
 */

#include <jelly/API.h>

#include <thread> // For std::this_thread::yield()

int
main(
	int		/*aNumArgs*/,
	char**	/*aArgs*/)
{
	// Define a host object.
	jelly::DefaultHost host(".", "example4", NULL);
	host.DeleteAllFiles();

	// Like the previous example we'll just use a blob node, but from a housekeeping
	// perspective lock nodes work exactly the same.
	typedef jelly::BlobNode<jelly::UIntKey<uint32_t>> BlobNodeType;
	BlobNodeType blobNode(&host, 4);            // Node id 4.

	// Create a housekeeping advisor object.
	typedef jelly::HousekeepingAdvisor<BlobNodeType> HousekeepingAdvisorType;
	HousekeepingAdvisorType housekeepingAdvisor(
		&host,                                  // The advisor will access the host.
		&blobNode);                             // Associate with a single node.


	// Define a blob.
	jelly::Blob blob;

	{
		blob.SetSize(5);
		memcpy(blob.GetPointer(), "hello", 5);
	}

	// We'll update the blob sequence number for every write.
	uint32_t seq = 0;

	// Loop until CTRL-C.
	for(;;)
	{
		// Continuously update the housekeeping advisor and it will call the 
		// supplied callback whenever it wants us to do something.
		housekeepingAdvisor.Update([&](
			const HousekeepingAdvisorType::Event& aEvent)
		{
			switch(aEvent.m_type)
			{
			// We should flush the pending WAL identified by m_concurrentWALIndex.
			// This event will be generated when there are pending writes in the 
			// WAL. Minimum interval can be configured with "min_wal_flush_interval",
			// defaults to 500 ms.
			// We need m_concurrentWALIndex in the case jelly has been configured
			// to have multiple parallel WALs.
			case HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_WAL:
				{
					printf("Flushing pending WAL: %u...\n", aEvent.m_concurrentWALIndex);

					size_t count = blobNode.FlushPendingWAL(aEvent.m_concurrentWALIndex);

					printf("(%zu items flushed)\n", count);
				}
				break;

			// Now is a good time to flush the pending store. This is triggered by
			// one of these circumstances:
			//   1. Number of (not necessarily unique) items in WAL files exceeds a 
			//      certain threshold. Configurable with "pending_store_wal_item_limit",
			//      defaults to 300000.
			//   2. Number of pending items exceed "pending_store_item_limit" (defaults
			//      to 30000). This is the number of unique items waiting to be written.
			//   3. Total size of all WALs exceed "pending_store_wal_size_limit" 
			//      (defaults to 128 MB). 
			case HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_STORE:
				{
					printf("Flushing pending store...\n");

					size_t count = blobNode.FlushPendingStore();

					printf("(%zu items flushed)\n", count);
				}
				break;

			// Unreferenced WALs should be deleted. This is usually suggested right
			// after a TYPE_FLUSH_PENDING_STORE event.
			case HousekeepingAdvisorType::Event::TYPE_CLEANUP_WALS:
				{
					printf("Cleaning up WALs...\n");

					size_t count = blobNode.CleanupWALs();

					printf("(%zu WALs deleted)\n", count);
				}
				break;

			// The number of stores on disk has reached a point where a compaction
			// would be a good idea. 
			case HousekeepingAdvisorType::Event::TYPE_PERFORM_COMPACTION:
				{
					printf("Performing compaction...\n");

					std::unique_ptr<BlobNodeType::CompactionResultType> compactionResult(
						blobNode.PerformCompaction(aEvent.m_compactionJob));

					blobNode.ApplyCompactionResult(compactionResult.get());
				}
				break;
			}
		});

		// Note that above we just call the appropriate Node methods directly
		// in the switch statement, but for performance reasons we could dispatch 
		// some of the calls to other threads. For simplicity it's done on the same
		// thread here.

		// Write a blob in every loop to give the advisor something to work with.
		// We'll keep writing to the same key, but with increasing sequence numbers.
		{
			BlobNodeType::Request req;
			req.SetKey(123);
			req.SetSeq(++seq);
			req.SetBlob(blob.Copy());
			blobNode.Set(&req);

			// Calling ProcessRequests() after each request is very inefficient. 
			// Better to call it on a steady interval, but let's keep it simple.
			blobNode.ProcessRequests();
			JELLY_ASSERT(req.GetResult() == jelly::REQUEST_RESULT_OK);
		}

		// Sleep a bit.
		std::this_thread::yield();
	}

	return EXIT_SUCCESS;
}