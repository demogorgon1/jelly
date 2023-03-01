/**
 * \example Example1_BlobNode.cpp
 * Basic example of how to save and load blobs.
 */

#include <jelly/API.h>

int
main(
	int		/*aNumArgs*/,
	char**	/*aArgs*/)
{
	// First we're going to need a host object. This provides our node an interface to talk to 
	// the system.
	//
	// jelly::DefaultHost provides a default, one size (mostly) fits all, implementation of the 
	// jelly::IHost interface, but you can implement your own if you're so inclined.
	jelly::DefaultHost host(
		".",									// Put database files here.
		"example1",								// Database files will be prefixed with this.
		NULL);									

	// Make sure the database is gone.
	host.DeleteAllFiles();

	// Make a typedef for our BlobNode type because we're lazy and don't want 
	// to type too much.
	typedef jelly::BlobNode
	<
		jelly::UIntKey<uint32_t>				// Our blob keys will be uint32_t.
	> BlobNodeType;

	// Create our blob node.
	BlobNodeType blobNode(
		&host,									// Associate the node with this host.
		1);										// Use globally unique node id 1.

	// Do a save
	{
		// Define a blob with the string "hello" inside
		jelly::Blob blob;

		{
			blob.SetSize(5);
			memcpy(blob.GetPointer(), "hello", 5);
		}

		// Define a request to save it.
		BlobNodeType::Request req;

		{
			// Set the blob key we want to save.
			req.SetKey(12345);

			// Set the sequence number of the blob. When the database encounters multiple 
			// instances of the same blob (identified by key), it will use the sequence number 
			// to determine which one is latest.
			// Note that the application is fully responsible for updating the sequence number
			// whenever it makes any changes to the blob.
			req.SetSeq(1);

			// Set the blob for the request. We need to copy the blob because ownership of the
			// object has to be transfered to the request.
			req.SetBlob(blob.Copy());
		}

		// Submit a "set" request to the blob node. The caller owns the request object and must
		// make sure it remains valid until the request has completed.
		blobNode.Set(&req);

		// At this point the request is queued inside the blob node. Now we'll make the blob node 
		// process all requests in the queue.
		blobNode.ProcessRequests();

		// Now the request has been written to the write-ahead-log (WAL), but it hasn't actually 
		// been written to disk yet. Because we're not sure that it's on disk yet, the request
		// hasn't been flagged as completed yet. 
		JELLY_ASSERT(!req.IsCompleted());

		// In order for this to happen we'll need to flush the pending WAL.
		blobNode.FlushPendingWAL();

		// Now the request will be completed.
		JELLY_ASSERT(req.IsCompleted());

		// REQUEST_RESULT_OK means everything went as expected.
		JELLY_ASSERT(req.GetResult() == jelly::REQUEST_RESULT_OK);
	}

	// Now try to load it back
	{
		// Define a request to load it.
		BlobNodeType::Request req;

		{
			// Set the blob key we want to load
			req.SetKey(12345);

			// Set the minimum blob sequence number we'll accept. We know we just saved with
			// sequence number 1, so let's demand it's at least that.
			req.SetSeq(1);			
		}

		// Submit a "get" request.
		blobNode.Get(&req);

		// Again we need to process requests before anything happens.
		blobNode.ProcessRequests();

		// Unlike set requests, get requests don't cause anything to be save to disk - so 
		// they'll be flagged as completed immediately after being processed.
		JELLY_ASSERT(req.IsCompleted());

		// Again REQUEST_RESULT_OK means everything is great.
		JELLY_ASSERT(req.GetResult() == jelly::REQUEST_RESULT_OK);

		// Read the blob and make sure we got the right data back.
		const jelly::IBuffer* blob = req.GetBlob();
		JELLY_ASSERT(blob->GetSize() == 5);
		JELLY_ASSERT(memcmp(blob->GetPointer(), "hello", 5) == 0);
		JELLY_UNUSED(blob);
	}

	return EXIT_SUCCESS;
}