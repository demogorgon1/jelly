#pragma once

#include "Request.h"

namespace jelly
{

	/**
	 * \brief Lock node request object. 
	 *
	 * You can use the LockNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType, typename _LockType, typename _LockMetaType>
	class LockNodeRequest
		: public Request<LockNodeRequest<_KeyType, _LockType, _LockMetaType>>
	{	
	public:
		LockNodeRequest() noexcept
			: m_forced(false)
			, m_seq(0)
		{

		}

		//! Set lock key for request.
		void
		SetKey(
			const _KeyType&			aKey) noexcept
		{
			m_key = aKey;
		}

		//! Set lock identifier for request.
		void
		SetLock(
			const _LockType&		aLock) noexcept
		{
			m_lock = aLock;
		}

		//! Set lock meta data.
		void
		SetMeta(
			const _LockMetaType&	aMeta) noexcept
		{
			m_meta = aMeta;
		}

		//! Try to force the request, regardles of existing lock.
		void
		SetForced(
			bool					aForced) noexcept
		{
			m_forced = aForced;
		}

		//! Set the lock sequence number
		void
		SetSeq(
			uint32_t				aSeq) noexcept
		{
			m_seq = aSeq;
		}

		//---------------------------------------------------------------------------------
		// Data access
		const _KeyType&			GetKey() const noexcept { return m_key; }		//!< Returns lock key
		const _LockType&		GetLock() const noexcept { return m_lock; }		//!< Returns lock identifier
		const _LockMetaType&	GetMeta() const noexcept { return m_meta; }		//!< Returns lock meta data
		bool					IsForced() const noexcept { return m_forced; }	//!< Returns whether request should be forced
		uint32_t				GetSeq() const noexcept { return m_seq; }		//!< Returns lock sequence number

	private:

		_KeyType							m_key;
		_LockType							m_lock;
		_LockMetaType						m_meta;
		bool								m_forced;
		uint32_t							m_seq;
	};

}