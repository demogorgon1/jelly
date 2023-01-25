#pragma once

#include "Request.h"

namespace jelly
{

	/**
	 * \brief Blob node request object. 
	 * 
	 * You can use the BlobNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType, typename _BlobType>
	class BlobNodeRequest
		: public Request<BlobNodeRequest<_KeyType, _BlobType>>
	{
	public:
		BlobNodeRequest()
			: m_seq(0)
		{

		}

		//! Set blob sequence number
		void
		SetSeq(
			uint32_t			aSeq)
		{
			m_seq = aSeq;
		}

		//! Set blob key
		void
		SetKey(
			const _KeyType&		aKey)
		{
			m_key = aKey;
		}

		//! Set blob object
		void
		SetBlob(
			const _BlobType&	aBlob)
		{
			m_blob = aBlob;
		}

		//-------------------------------------------------------------------------------
		// Data access

		uint32_t			GetSeq() const { return m_seq; }	//!< Get blob sequence number
		const _KeyType&		GetKey() const { return m_key; }	//!< Get blob key
		_BlobType&			GetBlob() { return m_blob; }		//!< Get reference to blob object
		const _BlobType&	GetBlob() const { return m_blob; }	//!< Get const reference to blob object

	private:

		uint32_t							m_seq;	
		_KeyType							m_key;	
		_BlobType							m_blob;	
	};

}