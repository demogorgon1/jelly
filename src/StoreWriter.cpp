#include <jelly/ErrorUtils.h>
#include <jelly/IItem.h>

#include "StoreWriter.h"

namespace jelly
{

	StoreWriter::StoreWriter(
		const char*						aPath)
		: m_file(aPath, File::MODE_WRITE_STREAM)
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
		const IItem*					aItem,
		const Compression::IProvider*	aItemCompression)
	{
		size_t offset = m_file.GetSize();

		File::Writer writer;
		m_file.GetWriter(writer);

		aItem->Write(&writer, aItemCompression);

		return offset;
	}

}