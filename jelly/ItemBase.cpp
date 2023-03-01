#include <jelly/Base.h>

#include <jelly/IReader.h>
#include <jelly/ItemBase.h>
#include <jelly/IWriter.h>

namespace jelly
{

	ItemBase::ItemBase()
	{

	}
	
	ItemBase::~ItemBase()
	{

	}

	bool			
	ItemBase::HasTombstone() const
	{
		return m_data.m_tombstoneStoreId != UINT32_MAX;
	}
	
	void			
	ItemBase::SetTombstoneStoreId(
		uint32_t						aTombstoneStoreId)
	{
		m_data.m_tombstoneStoreId = aTombstoneStoreId;
	}
	
	void			
	ItemBase::RemoveTombstone()
	{
		m_data.m_tombstoneStoreId = UINT32_MAX;
	}

	bool			
	ItemBase::ShouldBePruned(
		uint32_t						aCurrentOldestStoreId) const
	{
		return m_data.m_tombstoneStoreId != UINT32_MAX && aCurrentOldestStoreId > m_data.m_tombstoneStoreId;
	}

	void			
	ItemBase::SetTimeStamp(
		uint64_t						aTimeStamp)
	{
		m_data.m_timeStamp = aTimeStamp;
	}
	
	void			
	ItemBase::SetSeq(
		uint32_t						aSeq)
	{
		m_data.m_seq = aSeq;
	}

	void			
	ItemBase::IncrementSeq()
	{
		m_data.m_seq++;
	}

	//------------------------------------------------------------------------------------------

	void			
	ItemBase::ResetBase()
	{
		m_data.m_tombstoneStoreId = UINT32_MAX;
		m_data.m_timeStamp = 0;
		m_data.m_seq = 0;
	}

	void
	ItemBase::WriteBase(
		IWriter*						aWriter) const
	{
		aWriter->WritePOD(m_data);
	}
	
	bool			
	ItemBase::ReadBase(
		IReader*						aReader)
	{
		if (!aReader->ReadPOD(m_data))
			return false;
		return true;
	}

	void			
	ItemBase::CopyBase(
		const ItemBase&					aItemBase)
	{
		m_data.m_timeStamp = aItemBase.m_data.m_timeStamp;
		m_data.m_seq = aItemBase.m_data.m_seq;
		m_data.m_tombstoneStoreId = aItemBase.m_data.m_tombstoneStoreId;
	}

}