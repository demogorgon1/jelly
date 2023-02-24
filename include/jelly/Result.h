#pragma once

/**
 * \file Result.h
 */

namespace jelly
{

	/**
	 * Possible request results
	 */
	enum Result
	{
		RESULT_NONE,			   //!< No result yet
		RESULT_OK,				   //!< Request was completed succesfully
		RESULT_CANCELED,		   //!< Request was canceled
		RESULT_NOT_MASTER,		   //!< Request was submitted to a slave node
		RESULT_FAILED_TO_WRITE,	   //!< Failed to write to disk
		RESULT_ALREADY_LOCKED,	   //!< LockNode: the key was already locked with a different lock id
		RESULT_NOT_LOCKED,		   //!< LockNode: the key wasn't locked by the request lock id 
		RESULT_DOES_NOT_EXIST,	   //!< BlobNode: cannot get the blob as it doesn't exist
		RESULT_OUTDATED,		   //!< BlobNode: sequence number too low
		RESULT_FAILED_TO_READ,	   //!< BlobNode: couldn't read blob from disk
	};

}