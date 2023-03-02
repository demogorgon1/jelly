#include <jelly/API.h>

#include "UInt32Blob.h"

namespace jelly::Test::ErrorTest
{

	namespace 
	{
		void
		_SimulateSetFailure(
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

		void
		_SimulateStoreFlushFailure(
			Exception::Error		aError,
			Exception::Context		aExpectedContext)
		{
			DefaultHost host(".", "errortest");
			host.DeleteAllFiles(UINT32_MAX);
			typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
			BlobNodeType blobNode(&host, 0);

			{
				ErrorUtils::ResetErrorSimulation();

				BlobNodeType::Request req;
				req.SetKey(123);
				req.SetSeq(1); 
				req.SetBlob(new UInt32Blob(1001));
				blobNode.Set(&req);
				JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
				JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 1);
				JELLY_ALWAYS_ASSERT(req.IsCompleted());
				JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);

				// Make error happen once
				ErrorUtils::SimulateError(aError, 100, 1);

				try
				{
					blobNode.FlushPendingStore();
					JELLY_ALWAYS_ASSERT(false);
				}
				catch(Exception::Code e)
				{
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeError(e) == aError);
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeContext(e) == aExpectedContext);
				}				

				// Try to flush again, now it should work
				JELLY_ALWAYS_ASSERT(blobNode.FlushPendingStore() == 1);
			}
		}

		void
		_SimulateGetFailure(
			Exception::Error		aError,
			Exception::Context		aExpectedContext,
			Exception::RequestType	aExpectedRequestType)
		{
			DefaultConfigSource config;
			config.Set(Config::ID_MAX_RESIDENT_BLOB_COUNT, "0");

			DefaultHost host(".", "errortest", &config);
			host.DeleteAllFiles(UINT32_MAX);
			typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
			BlobNodeType blobNode(&host, 0);

			{
				ErrorUtils::ResetErrorSimulation();

				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(1000));
					blobNode.Set(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ALWAYS_ASSERT(blobNode.FlushPendingWAL() == 1);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					JELLY_ALWAYS_ASSERT(blobNode.FlushPendingStore() == 1);
				}

				// Make error happen once
				ErrorUtils::SimulateError(aError, 100, 1);

				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(1);
					blobNode.Get(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_EXCEPTION);
					uint32_t exceptionCode = req.GetException();
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeError(exceptionCode) == aError);
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeContext(exceptionCode) == aExpectedContext);
					JELLY_ALWAYS_ASSERT(Exception::GetExceptionCodeRequestType(exceptionCode) == aExpectedRequestType);
				}

				// Try again, it should work
				{
					BlobNodeType::Request req;
					req.SetKey(123);
					req.SetSeq(1);
					blobNode.Get(&req);
					JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ALWAYS_ASSERT(req.IsCompleted());
					JELLY_ALWAYS_ASSERT(req.GetResult() == REQUEST_RESULT_OK);
					JELLY_ALWAYS_ASSERT(UInt32Blob::GetValue(req.GetBlob()) == 1000);
				}
			}
		}
	}

	void		
	Run()
	{
		_SimulateSetFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_SET);
		_SimulateSetFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE, Exception::CONTEXT_NODE_FLUSH_PENDING_WAL, Exception::REQUEST_TYPE_NONE);
		_SimulateSetFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_FLUSH, Exception::CONTEXT_NODE_FLUSH_PENDING_WAL, Exception::REQUEST_TYPE_NONE);

		#if defined(JELLY_ZSTD)
			_SimulateSetFailure(Exception::ERROR_ZSTD_BUFFER_TOO_LARGE_TO_COMPRESS, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_SET);
			_SimulateSetFailure(Exception::ERROR_ZSTD_BUFFER_COMPRESSION_FAILED, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_SET);
		#endif
	
		_SimulateStoreFlushFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN, Exception::CONTEXT_NODE_FLUSH_PENDING_STORE);
		_SimulateStoreFlushFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE, Exception::CONTEXT_NODE_FLUSH_PENDING_STORE);
		_SimulateStoreFlushFailure(Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_FLUSH, Exception::CONTEXT_NODE_FLUSH_PENDING_STORE);

		_SimulateGetFailure(Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_READ, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);
		_SimulateGetFailure(Exception::ERROR_FILE_READ_RANDOM_HEADER_MISMATCH, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);
		_SimulateGetFailure(Exception::ERROR_FAILED_TO_GET_BLOB_READER, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);

		#if defined(JELLY_ZSTD)
			_SimulateGetFailure(Exception::ERROR_ZSTD_BUFFER_INVALID, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);
			_SimulateGetFailure(Exception::ERROR_ZSTD_BUFFER_TOO_LARGE_TO_UNCOMPRESS, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);
			_SimulateGetFailure(Exception::ERROR_ZSTD_BUFFER_DECOMPRESSION_FAILED, Exception::CONTEXT_NODE_PROCESS_REQUESTS, Exception::REQUEST_TYPE_BLOB_NODE_GET);
		#endif
	}

}