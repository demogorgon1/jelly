#include <jelly/API.h>

#include "UInt32Blob.h"

namespace jelly::Test::ErrorTest
{

	namespace 
	{
		void
		_SimulateWALFailure(
			Exception::Error		aError,
			Exception::Context		aExpectedContext,
			Exception::RequestType	aExpectedRequestType)
		{
			DefaultHost host(".", "errortest");
			host.DeleteAllFiles(UINT32_MAX);
			typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
			BlobNodeType blobNode(&host, 0);

			{
				ErrorUtils::ResetErrorSimulation();

				// Make error happen once
				ErrorUtils::SimulateError(aError, 100, 1);

				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(1000));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_EXCEPTION);
					uint32_t exceptionCode = req.GetException();
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeError(exceptionCode) == aError);
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeContext(exceptionCode) == aExpectedContext);
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeRequestType(exceptionCode) == aExpectedRequestType);
				}

				// Try again, this time it should work
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(2); // Note we have to bump sequence number because we don't know if it was actually updated or not
					req.SetBlob(new UInt32Blob(1001));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					blobNode.FlushPendingWAL();
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
				}

				// Read back to verify
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(2);
					blobNode.Get(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1001);
				}
			}

		}
	}

	void		
	Run()
	{
		_SimulateWALFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_SET);
		_SimulateWALFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE, Exception::CONTEXT_NODE_FLUSH_PENDING_WAL, Exception::REQUEST_TYPE_NONE);
	}

}