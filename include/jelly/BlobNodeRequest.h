#pragma once

#include "Request.h"

namespace jelly
{

	// Request for BlobNode
	template <typename _KeyType, typename _BlobType>
	struct BlobNodeRequest
		: public Request<BlobNodeRequest<_KeyType, _BlobType>>
	{
		BlobNodeRequest()
			: m_seq(0)
		{

		}
		
		// See explanations in BlobNode
		uint32_t							m_seq;
		_KeyType							m_key;
		_BlobType							m_blob;
	};

}