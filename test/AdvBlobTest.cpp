#include <jelly/API.h>

#include "UInt32Blob.h"

namespace jelly::Test::AdvBlobTest
{

	void
	_TestUIntVectorKeySorting()
	{
		// See that sorting works as expected
		std::vector<UIntVectorKey<uint32_t, 2>> keys = 
		{
			{ 2, 5 },
			{ 1, 2 },
			{ 1, 3 },			 
			{ 1, 1 }
		};

		std::sort(keys.begin(), keys.end());

		std::vector<UIntVectorKey<uint32_t, 2>> expected =
		{
			{ 1, 1 },
			{ 1, 2 },
			{ 1, 3 },
			{ 2, 5 }
		};

		JELLY_ALWAYS_ASSERT(keys == expected);
	}

	void		
	Run()
	{
		_TestUIntVectorKeySorting();

		// Test vector keys and blob meta data
		DefaultHost host(".", "advblobtest");
		host.DeleteAllFiles();

		typedef BlobNode<UIntVectorKey<uint32_t, 2>> BlobNodeType;

		{
			BlobNodeType blobNode(&host, 0);
			
			// Set a blob identified by a vector key
			{
				BlobNodeType::Request req;
				req.SetKey({ 1, 2 });
				req.SetSeq(1);
				req.SetBlob(new UInt32Blob(123));
				blobNode.Set(&req);
				JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
				JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 1);
				JELLY_ALWAYS_ASSERT(req.IsCompleted());
				JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);
			}

			// Read it back
			{
				BlobNodeType::Request req;
				req.SetKey({ 1, 2 });
				req.SetSeq(1);
				blobNode.Get(&req);
				JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
				JELLY_ALWAYS_ASSERT(req.IsCompleted());
				JELLY_ALWAYS_ASSERT(req.GetResult() == RESULT_OK);
				JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 123);
			}
		}
	}

}
