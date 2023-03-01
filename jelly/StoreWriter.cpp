#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/FileHeader.h>
#include <jelly/ItemBase.h>
#include <jelly/Stat.h>

#include "StoreWriter.h"

namespace jelly
{

	StoreWriter::StoreWriter(
		const char*						aTargetPath,
		const char*						aTempPath,
		FileStatsContext*				aFileStatsContext,
		const FileHeader&				aFileHeader)
		: m_file(aFileStatsContext, aTempPath, File::MODE_WRITE_STREAM, aFileHeader)
		, m_targetPath(aTargetPath)
		, m_tempPath(aTempPath)
		, m_isFlushed(false)
	{
		JELLY_ASSERT(aFileHeader.m_type == FileHeader::TYPE_STORE);
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
		const ItemBase*					aItem)
	{
		size_t offset = m_file.GetSize();

		return offset + aItem->Write(&m_file);
	}

	void
	StoreWriter::Flush() 
	{
		JELLY_ASSERT(!m_isFlushed);

		m_file.Flush();
		m_file.Close();

		// Rename temp path to final target path
		std::error_code errorCode;
		std::filesystem::rename(m_tempPath, m_targetPath, errorCode);
		JELLY_CHECK(!errorCode, Result::ERROR_STORE_WRITER_RENAME_FAILED, "Temp=%s;Target=%s;Msg=%s", m_tempPath.c_str(), m_targetPath.c_str(), errorCode.message().c_str());

		m_isFlushed = true;
	}

}