/**
 * \example Example2_LockNode.cpp
 * Basic example of how to lock and unlock a key
 */

#include <jelly/API.h>

int
main(
	int		/*aNumArgs*/,
	char**	/*aArgs*/)
{
	// Like with the previous example, we'll need a host object.
	jelly::DefaultHost host(
		".",									// Put database files here.
		"example2",								// Database files will be prefixed with this.
		NULL);									// Use default configuration.

	// Make sure the database is gone.
	host.DeleteAllFiles();

	// Define the type of additional meta data we'll associate with each lock key.
	// StaticSingleBlob contains two pieces of information:
	// 
	//   1. What the latest blob sequence number is.
	//   2. Which blob nodes have a copy of the latest blob.
	// 
	// StaticSingleBlob assumes that lock and blob keys are the same, but this doesn't need 
	// to be the case. You could, for example, use some kind of account id as lock key and 
	// then have a seperate blob for each profile associated with it.
	typedef jelly::LockMetaData::StaticSingleBlob<> LockMetaType;

	// Define the type of our LockNode
	typedef jelly::LockNode
	<
		jelly::UIntKey<uint32_t>,				// Our lock keys will be uint32_t.
		jelly::UIntLock<uint32_t>,				// Our lock identifiers will also be uint32_t.
		LockMetaType							// The meta data type we defined before.
	> LockNodeType;

	// Create our lock node.
	LockNodeType lockNode(
		&host,									// Associate the node with this host.
		2);										// Use globally unique node id 2.

	// Lock a key
	{
		// Define the request
		LockNodeType::Request req;

		{
			// Set the lock key, identifying the object we want to lock (in this case a blob
			// with the same key).
			req.SetKey(12345);

			// Set the lock identifier, identifying the process that is applying the lock. All
			// process (i.e. usually game servers) will need to have a globally unique lock
			// identifier.
			// If the key is already locked by some other lock identifier, the request will
			// fail. If the key is locked with the same lock identifier, it will succeed.
			req.SetLock(100);
		}

		// Submit the "lock" request.
		lockNode.Lock(&req);

		// Tell the node to process requests.
		lockNode.ProcessRequests();

		// The lock hasn't been flushed to disk yet, so the request hasn't been flagged as
		// completed.
		JELLY_ASSERT(!req.IsCompleted());

		// Flush the pending WAL (which is currently just our lock),
		lockNode.FlushPendingWAL();

		// Now the request will be completed.
		JELLY_ASSERT(req.IsCompleted());

		// RESULT_OK means everything went as expected.
		JELLY_ASSERT(req.GetResult() == jelly::RESULT_OK);

		// Meta data is returned when the lock is successful. Since this is a new lock, we'll
		// see that the meta data is blank.
		const LockMetaType& meta = req.GetMeta();

		JELLY_ASSERT(meta.m_blobNodeIdCount == 0);	// Associated blob isn't anywhere.
		JELLY_ASSERT(meta.m_blobSeq == UINT32_MAX); // No blob sequence number.
		JELLY_UNUSED(meta);
	}

	// Now unlock the same key
	{
		// Define a request to unlock
		LockNodeType::Request req;

		{
			// Set the lock key we want to unlock
			req.SetKey(12345);

			// Again we'll set our lock identifier. Unlock requests will fail if the lock
			// identifier isn't the same one that was used to apply the lock.
			req.SetLock(100);		
			
			// We'll also update the meta data, pretending that we saved the associated blob
			// on some blob node.
			req.SetMeta(LockMetaType(
				1,			// The latest blob sequence number we saved...
				{ 1 }));	// ... and the blob node ids where we pretended to save it.
		}

		// Submit the "unlock" request.
		lockNode.Unlock(&req);

		// Again we need to process requests before anything happens.
		lockNode.ProcessRequests();

		// Like the lock request, the unlock request won't be flagged as completted before
		// the pending WAL has been flushed to disk.
		JELLY_ASSERT(!req.IsCompleted());

		// Flush it ...
		lockNode.FlushPendingWAL();

		// ... and now it's completed.
		JELLY_ASSERT(req.IsCompleted());

		// Again RESULT_OK means everything is great.
		JELLY_ASSERT(req.GetResult() == jelly::RESULT_OK);
	}

	return EXIT_SUCCESS;
}