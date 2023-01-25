#pragma once

#include "Request.h"

namespace jelly
{

	/**
	 * \brief Lock node request object. 
	 *
	 * You can use the LockNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType, typename _LockType>
	class LockNodeRequest
		: public Request<LockNodeRequest<_KeyType, _LockType>>
	{	
	public:
		LockNodeRequest()
			: m_blobSeq(0)
			, m_blobNodeIds(UINT32_MAX)
		{

		}

		//! Set lock key for request
		void
		SetKey(
			const _KeyType&			aKey)
		{
			m_key = aKey;
		}

		//! Set lock identifier for request
		void
		SetLock(
			const _LockType&		aLock)
		{
			m_lock = aLock;
		}

		void
		SetBlobSeq(
			uint32_t				aBlobSeq)
		{
			m_blobSeq = aBlobSeq;
		}

		void
		SetBlobNodeIds(
			uint32_t				aBlobNodeIds)
		{
			m_blobNodeIds = aBlobNodeIds;
		}

		//---------------------------------------------------------------------------------
		// Data access

		const _KeyType&		GetKey() const { return m_key; }					//!< Returns lock key
		const _LockType&	GetLock() const { return m_lock; }					//!< Returns lock identifier
		uint32_t			GetBlobSeq() const { return m_blobSeq; }			
		uint32_t			GetBlobNodeIds() const { return m_blobNodeIds; }

	private:

		_KeyType							m_key;
		_LockType							m_lock;
		uint32_t							m_blobSeq;
		uint32_t							m_blobNodeIds;
	};

}