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
	template <typename _KeyType, typename _MetaType>
	class BlobNodeRequest
		: public Request<BlobNodeRequest<_KeyType, _MetaType>>
	{
	public:
		BlobNodeRequest()
			: m_seq(0)
		{

		}

		//! Set blob sequence number
		void
		SetSeq(
			uint32_t			aSeq) noexcept
		{
			m_seq = aSeq;
		}

		//! Set blob key
		void
		SetKey(
			const _KeyType&		aKey) noexcept
		{
			m_key = aKey;
		}

		//! Set blob object
		void
		SetBlob(
			IBuffer*			aBuffer) noexcept
		{
			m_blob.reset(aBuffer);
		}

		//! Set meta data
		void
		SetMeta(
			const _MetaType&	aMeta) noexcept
		{
			m_meta = aMeta;
		}

		//-------------------------------------------------------------------------------
		// Data access

		uint32_t						GetSeq() const noexcept { return m_seq; }			//!< Get blob sequence number
		const _KeyType&					GetKey() const noexcept { return m_key; }			//!< Get blob key
		const std::optional<_MetaType>&	GetMeta() const noexcept { return m_meta; }			//!< Get meta data
		IBuffer*						GetBlob() noexcept { return m_blob.get(); }			//!< Get pointer to blob object
		const IBuffer*					GetBlob() const noexcept { return m_blob.get(); }	//!< Get const pointer to blob object
		IBuffer*						DetachBlob() noexcept { return m_blob.release(); }	//!< Detach blob object from request

	private:

		uint32_t							m_seq;	
		_KeyType							m_key;	
		std::optional<_MetaType>			m_meta;
		std::unique_ptr<IBuffer>			m_blob;	
	};

}