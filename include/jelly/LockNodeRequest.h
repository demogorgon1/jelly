#pragma once

#include "Request.h"

namespace jelly
{

	/**
	 * Lock node request object. You can use the LockNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType, typename _LockType>
	struct LockNodeRequest
		: public Request<LockNodeRequest<_KeyType, _LockType>>
	{	
		LockNodeRequest()
			: m_blobSeq(0)
			, m_blobNodeIds(UINT32_MAX)
		{

		}

		//----------------------------------------------------------------------
		// Public data

		// See explanations in LockNode
		_KeyType							m_key;			//!< Lock key		
		_LockType							m_lock;			//!< Lock identifier
		uint32_t							m_blobSeq;		//!< Latest sequence number of associated blob		
		uint32_t							m_blobNodeIds;	//!< 4 packed uint8_t, 0xFF = not set
	};

}