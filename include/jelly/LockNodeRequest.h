#pragma once

#include "Request.h"

namespace jelly
{

	// Request for LockNode
	template <typename _KeyType, typename _LockType>
	struct LockNodeRequest
		: public Request<LockNodeRequest<_KeyType, _LockType>>
	{	
		LockNodeRequest()
			: m_blobSeq(0)
			, m_blobNodeIds(UINT32_MAX)
		{

		}

		// See explanations in LockNode
		_KeyType							m_key;			
		_LockType							m_lock;			
		uint32_t							m_blobSeq;		
		uint32_t							m_blobNodeIds; // 4 packed uint8_t, 0xFF = not set
	};

}