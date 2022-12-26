#include <assert.h>

#include <jelly/IItem.h>

#include "StoreWriter.h"

namespace jelly
{

	namespace Impl
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

			if (!aItem->Write(&writer, aItemCompression))
				assert(false);

			return offset;
		}

	}

}