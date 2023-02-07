/**
 * \example Example3_Housekeeping.cpp
 * Illustrates how to do housekeeping operations.
 */

#include <jelly/API.h>

int
main(
	int		/*aNumArgs*/,
	char**	/*aArgs*/)
{
	// Again, we'll need a host.
	jelly::DefaultHost host(
		".",									// Put database files here.
		"example3",								// Database files will be prefixed with this.
		NULL,									// Use default configuration.
		jelly::Compression::ID_NO_COMPRESSION);	// Don't use compression.

	// Make sure the database is gone.
	host.DeleteAllFiles();

	// In this example we'll use a blob node, but in terms of housekeeping you'll see that 
	// both kinds of nodes work exactly the same way.
	typedef jelly::BlobNode
	<
		jelly::UIntKey<uint32_t>,				// Our blob keys will be uint32_t.
		jelly::Blob<>							// Our blobs will be jelly::Blob.
	> BlobNodeType;

	// Create our blob node.
	BlobNodeType blobNode(
		&host,									// Associate the node with this host.
		3);										// Use globally unique node id 3.

	// Define a blob.
	jelly::Blob<> blob;

	{
		blob.GetBuffer().SetSize(5);
		memcpy(blob.GetBuffer().GetPointer(), "hello", 5);
	}

	// Right, let's start off by saving this blob a bunch of times under the same key. 
	// Imagine that this is a blob that contains the savegame of a player, saved at 
	// a regular interval or when something needs to be saved.
	static constexpr uint32_t NUM_SAVES = 2000;
	BlobNodeType::Request reqs[NUM_SAVES];

	for (uint32_t i = 0; i < NUM_SAVES; i++)
	{
		// Define the request
		reqs[i].SetKey(123);
		reqs[i].SetSeq(1 + i); // Incrementing sequence numbers.
		reqs[i].SetBlob(blob);

		// Submit a set request
		blobNode.Set(&reqs[i]);
	}

	// Process all the requests.
	blobNode.ProcessRequests();

	// Flush them all to disk. Note that this time we're flushing thousands of requests
	// at the same time. In general that's what we need to do: we shouldn't flush
	// individual requests, we need to batch as many as possible at the same time.
	// This is much more efficient. 
	blobNode.FlushPendingWAL();

	// All the requests have now been flushed to disk and flagged as completed.
	for (uint32_t i = 0; i < NUM_SAVES; i++)
	{
		JELLY_ASSERT(reqs[i].IsCompleted());
		JELLY_ASSERT(reqs[i].GetResult() == jelly::RESULT_OK);
	}

	// If you look at your file system at this point, you'll find that there is 
	// just a WAL file. If we just continued to make more set requests, these will 
	// be appended to the WAL, which will grow and grow forever.
	// Obviously, this is not something we want. In this example we only have 1 key,
	// but on disk (in the WAL) we have thousands of copies of the blob associated 
	// with it. We only care about the latest version of the blob, the one with the
	// highest sequence number.

	// The "pending store" is the set of all items (in this case blobs) that currently
	// have a write in the WAL. In this particular case, the pending store is just a 
	// single item (blob key 123), but it has thousands of "WAL instances". Every item
	// in the pending store holds a reference to the WAL where the highest sequence 
	// number instance is written. 

	// What we need to do now is to flush the pending store to disk. 
	blobNode.FlushPendingStore();

	// Now you'll see another file on disk, a "store". This particular store contains 
	// just a single item, but because the "pending store" is now empty, no items hold
	// any references to any WALs at this point. The single WAL we have has a 
	// reference count of zero.

	// WALs with no references (i.e. they don't hold the latest sequence number of
	// any pending store items) can be cleaned up (removed). Let's try that.
	blobNode.CleanupWALs();

	// What? We still have our WAL! Yes we do. That's because the WAL is still "open",
	// meaning that this is the WAL any incoming requests will write their data to.
	// As soon as another request writes to it, its reference count will be 
	// incremented again.
	// This WAL will continue to grow until it reaches a certain size (defaults to 
	// 64 MB), at which point it will be closed and a new WAL will be created. 
	// When it's closed and there are no references to it, it can be cleaned up.

	// "Stores" are immutable files stored on disk. Every time the pending store
	// is flushed it will create a new store file (provided the pending store wasn't
	// empty). You can't just flush pending store all the time, as it will create
	// large number of small files - but even if we're smart about when we call it,
	// we'll still see a growing number of store files. Even if we clean up WALs
	// regularly, disk space will still continue to grow.

	// This is where "compaction" comes into play. It will combine smaller stores 
	// into bigger one, removing superflous copies of items in the process. 
	// The latest store can never be compacted (because it might be in process of
	// writing), so we'll always need at least 3 stores.

	// Let's make two more stores, so we can do a compaction.
	for(uint32_t i = 1; i <= 2; i++)
	{
		BlobNodeType::Request req;
		req.SetKey(123);			// Still using the same key...
		req.SetSeq(NUM_SAVES + i);	// ... but with the highest sequence number.
		req.SetBlob(blob);			// Set our test blob.

		// Submit, process, and flush to make another store.
		blobNode.Set(&req);
		blobNode.ProcessRequests();
		blobNode.FlushPendingWAL();
		blobNode.FlushPendingStore();
	}

	// Now we have 3 stores. Each of them are identified with a "store id", which
	// is an ever-increasing uint32_t. First store (store id 0) is the original one
	// while store id 1 and 2 contains the two new blobs we just flushed.

	// We'll now do a "minor" compaction of store ids 0 and 1. Minor compactions 
	// take two or more specific stores and turn them into one. Major compactions 
	// will compact all stores, but should be considered more of an off-line 
	// process.
	{
		jelly::CompactionJob compactionJob;

		// Tell the compaction process that the oldest store in disk is store id 0.
		// This information helps it decide which tombstones to prune (more on that
		// some other time).
		compactionJob.m_oldestStoreId = 0;	

		compactionJob.m_storeIds = { 0, 1 };

		// Perform the minor compaction. Note that this can take a while and is a 
		// blocking call (like everything else), but luckily you're free to call
		// this on another worker thread. You can have any number of minor
		// compactions happening at the same time, as long as the don't touch the 
		// same stores.
		std::unique_ptr<BlobNodeType::CompactionResultType> compactionResult(
			blobNode.PerformCompaction(compactionJob));

		// The compaction is completed, but it hasn't to be applied yet. The main
		// implication of this is that store id 0 and 1 still exists on disk.
		// So, let's apply the result:
		blobNode.ApplyCompactionResult(compactionResult.get());

		// Now you'll see that we have two stores: store id 2 (unaffected by 
		// compaction) and store id 3 (former store id 0 and 1 combined).
	}
	
	// There is a lot of "devil in the details" about deciding when to perform these
	// various housekeeping tasks. If you do them too much it will negatively affect
	// performance, but if you do them too little, you'll end up with a lot of 
	// superflous data on disk.

	// Next example will be about the housekeeping advisor, which can help 
	// monitoring you node and suggest when to perform various housekeeping tasks.

	return EXIT_SUCCESS;
}