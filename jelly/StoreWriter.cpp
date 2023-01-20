#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/IItem.h>
#include <jelly/Stat.h>

#include "StoreWriter.h"

namespace jelly
{

	StoreWriter::StoreWriter(
		const char*						aPath,
		FileStatsContext*				aFileStatsContext)
		: m_file(aFileStatsContext, aPath, File::MODE_WRITE_STREAM)
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
		m_file.Flush();
	}

}