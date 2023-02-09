#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/ItemBase.h>
#include <jelly/Stat.h>

#include "StoreWriter.h"

namespace jelly
{

	StoreWriter::StoreWriter(
		const char*						aTargetPath,
		const char*						aTempPath,
		FileStatsContext*				aFileStatsContext)
		: m_file(aFileStatsContext, aTempPath, File::MODE_WRITE_STREAM)
		, m_targetPath(aTargetPath)
		, m_tempPath(aTempPath)
	{
	}

	StoreWriter::~StoreWriter()
	{
		m_file.Close();

		// Rename temp path to final target path
		std::error_code errorCode;
		std::filesystem::rename(m_tempPath, m_targetPath, errorCode);
		JELLY_CHECK(!errorCode, "Rename failed: %s %s", m_tempPath.c_str(), m_targetPath.c_str());
	}

	bool
	StoreWriter::IsValid() const
	{
		return m_file.IsValid();
	}
			
	//-------------------------------------------------------------------------------
		
	size_t
	StoreWriter::WriteItem(
		const ItemBase*					aItem)
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