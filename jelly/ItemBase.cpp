#include <jelly/Base.h>

#include <jelly/IReader.h>
#include <jelly/ItemBase.h>
#include <jelly/IWriter.h>

namespace jelly
{

	ItemBase::ItemBase()
		: m_tombstoneStoreId(UINT32_MAX)
		, m_seq(0)
		, m_timeStamp(0)
	{

	}
	
	ItemBase::~ItemBase()
	{

	}

	bool			
	ItemBase::HasTombstone() const
	{
		return m_tombstoneStoreId != UINT32_MAX;
	}
	
	void			
	ItemBase::SetTombstoneStoreId(
		uint32_t						aTombstoneStoreId)
	{
		m_tombstoneStoreId = aTombstoneStoreId;
	}
	
	void			
	ItemBase::RemoveTombstone()
	{
		m_tombstoneStoreId = UINT32_MAX;
	}

	bool			
	ItemBase::ShouldBePruned(
		uint32_t						aCurrentOldestStoreId) const
	{
		return m_tombstoneStoreId != UINT32_MAX && aCurrentOldestStoreId > m_tombstoneStoreId;
	}

	void			
	ItemBase::SetTimeStamp(
		uint64_t						aTimeStamp)
	{
		m_timeStamp = aTimeStamp;
	}
	
	void			
	ItemBase::SetSeq(
		uint32_t						aSeq)
	{
		m_seq = aSeq;
	}

	void			
	ItemBase::IncrementSeq()
	{
		m_seq++;
	}

	//------------------------------------------------------------------------------------------

	void			
	ItemBase::ResetBase()
	{
		m_tombstoneStoreId = UINT32_MAX;
		m_timeStamp = 0;
		m_seq = 0;
	}

	void
	ItemBase::WriteBase(
		IWriter*						aWriter) const
	{
		JELLY_CHECK(aWriter->WritePOD(m_tombstoneStoreId), "Failed to write tombstone store id.");
		JELLY_CHECK(aWriter->WriteUInt(m_timeStamp), "Failed to write time stamp.");
		JELLY_CHECK(aWriter->WriteUInt(m_seq), "Failed to write sequence number.");
	}
	
	bool			
	ItemBase::ReadBase(
		IReader*						aReader)
	{
		if (!aReader->ReadPOD(m_tombstoneStoreId))
			return false;
		if (!aReader->ReadUInt(m_timeStamp))
			return false;
		if (!aReader->ReadUInt(m_seq))
			return false;
		return true;
	}

	bool			
	ItemBase::CompareBase(
		const ItemBase&					aItemBase) const
	{
		return m_tombstoneStoreId == aItemBase.m_tombstoneStoreId && m_seq == aItemBase.m_seq; // Don't compare timestamp
	}

	void			
	ItemBase::CopyBase(
		const ItemBase&					aItemBase)
	{
		m_timeStamp = aItemBase.m_timeStamp;
		m_seq = aItemBase.m_seq;
		m_tombstoneStoreId = aItemBase.m_tombstoneStoreId;
	}

}