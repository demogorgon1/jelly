#pragma once

#include <assert.h>

#include "IWALWriter.h"

namespace jelly
{

	// This class represents a write-ahead log (WAL) that hasn't been flushed to a store yet (i.e. it's still 
	// part of the "pending store"). The reference count represents the number of items that has their latest
	// version (by sequence number) stored in this WAL. If the reference count reaches zero it means that the 
	// WAL can be cleaned up (deleted).
	class WAL
	{
	public:
		WAL(
			uint32_t			aId,
			IWALWriter*			aWriter)
			: m_id(aId)
			, m_writer(aWriter)
			, m_refCount(0)
		{

		}
		
		~WAL()
		{

		}

		void
		AddReference()
		{
			m_refCount++;
		}

		void
		RemoveReference()
		{
			assert(m_refCount > 0);
			m_refCount--;
		}

		void
		Close()
		{
			assert(m_writer);
			m_writer = NULL;
		}

		// Data access
		uint32_t	GetId() const { return m_id; }
		uint32_t	GetRefCount() const { return m_refCount; }
		bool		IsClosed() const { return !m_writer; }
		IWALWriter*	GetWriter() { return m_writer.get(); }

	private:

		uint32_t							m_id;
		std::unique_ptr<IWALWriter>			m_writer;
		uint32_t							m_refCount;
	};

}