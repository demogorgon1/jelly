#pragma once

#include "IBuffer.h"
#include "Request.h"

namespace jelly
{

	/**
	 * \brief Blob node request object. 
	 * 
	 * You can use the BlobNode::Request typedef as a shortcut to this.
	 */
	template <typename _KeyType>
	class BlobNodeRequest
		: public Request<BlobNodeRequest<_KeyType>>
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
			IBuffer*			aBuffer)
		{
			m_blob.reset(aBuffer);
		}

		//-------------------------------------------------------------------------------
		// Data access

		uint32_t			GetSeq() const { return m_seq; }			//!< Get blob sequence number
		const _KeyType&		GetKey() const { return m_key; }			//!< Get blob key
		IBuffer*			GetBlob() { return m_blob.get(); }			//!< Get pointer to blob object
		const IBuffer*		GetBlob() const { return m_blob.get(); }	//!< Get const pointer to blob object
		IBuffer*			DetachBlob() { return m_blob.release(); }	//!< Detach blob object from request

	private:

		uint32_t							m_seq;	
		_KeyType							m_key;	
		std::unique_ptr<IBuffer>			m_blob;	
	};

}