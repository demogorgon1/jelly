#pragma once

#include "Request.h"

namespace jelly
{

	/**
	 * Blob node request object. You can use the BlobNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType, typename _BlobType>
	struct BlobNodeRequest
		: public Request<BlobNodeRequest<_KeyType, _BlobType>>
	{
		BlobNodeRequest()
			: m_seq(0)
		{

		}

		//----------------------------------------------------------------------
		// Public data

		// See explanations in BlobNode
		uint32_t							m_seq;	//!< Blob sequence number
		_KeyType							m_key;	//!< Blob key
		_BlobType							m_blob;	//!< Blob object
	};

}