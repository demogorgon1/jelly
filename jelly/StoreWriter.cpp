#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/IItem.h>
#include <jelly/Stat.h>

#include "StoreWriter.h"

namespace jelly
{

	StoreWriter::StoreWriter(
		IStats*							aStats,
		const char*						aPath)
		: m_file(aPath, File::MODE_WRITE_STREAM)
		, m_stats(aStats)
	{
	}

	StoreWriter::~StoreWriter()
	{
	}

	bool
	StoreWriter::IsValid() const
	{
		return m_file.IsValid();
	}
			
	//-------------------------------------------------------------------------------
		
	size_t
	StoreWriter::WriteItem(
		const IItem*					aItem)
	{
		size_t offset = m_file.GetSize();

		return offset + aItem->Write(&m_file);
	}

	void
	StoreWriter::Flush() 
	{
		size_t flushedBytes = m_file.Flush();

		m_stats->AddCounter(Stat::COUNTER_DISK_WRITE_STORE_BYTES, flushedBytes);
	}

}