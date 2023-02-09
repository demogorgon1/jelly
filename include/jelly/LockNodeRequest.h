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
		LockNodeRequest()
			: m_forced(false)
		{

		}

		//! Set lock key for request.
		void
		SetKey(
			const _KeyType&			aKey)
		{
			m_key = aKey;
		}

		//! Set lock identifier for request.
		void
		SetLock(
			const _LockType&		aLock)
		{
			m_lock = aLock;
		}

		//! Set lock meta data.
		void
		SetMeta(
			const _LockMetaType&	aMeta)
		{
			m_meta = aMeta;
		}

		//! Try to force the request, regardles of existing lock.
		void
		SetForced(
			bool					aForced)
		{
			m_forced = aForced;
		}

		//---------------------------------------------------------------------------------
		// Data access
		const _KeyType&			GetKey() const { return m_key; }		//!< Returns lock key
		const _LockType&		GetLock() const { return m_lock; }		//!< Returns lock identifier
		const _LockMetaType&	GetMeta() const { return m_meta; }		//!< Returns lock meta data
		bool					IsForced() const { return m_forced;	}	//!< Returns whether request should be forced

	private:

		_KeyType							m_key;
		_LockType							m_lock;
		_LockMetaType						m_meta;
		bool								m_forced;
	};

}