#pragma once

/**
 * \file RequestResult.h
 */

namespace jelly
{

	/**
	 * Possible request results
	 */
	enum RequestResult
	{
		REQUEST_RESULT_NONE,				//!< No result yet
		REQUEST_RESULT_OK,					//!< Request was completed succesfully
		REQUEST_RESULT_CANCELED,			//!< Request was canceled
		REQUEST_RESULT_NOT_MASTER,			//!< Request was submitted to a slave node
		REQUEST_RESULT_EXCEPTION,			//!< Error occured while processing request (call Request::GetError() for details)
		REQUEST_RESULT_NOT_LOCKED,			//!< Not locked be the requester
		REQUEST_RESULT_ALREADY_LOCKED,		//!< LockNode: the key was already locked with a different lock id
		REQUEST_RESULT_DOES_NOT_EXIST,		//!< BlobNode: cannot get the blob as it doesn't exist
		REQUEST_RESULT_OUTDATED,			//!< BlobNode: sequence number too low
	};

}