#include <jelly/CompletionEvent.h>
#include <jelly/Compression.h>
#include <jelly/ErrorUtils.h>
#include <jelly/IItem.h>

#include "WALWriter.h"

namespace jelly
{

	WALWriter::WALWriter(
		const char*						aPath,
		Compression::IStreamCompressor* aCompressor)
		: m_file(aPath, File::MODE_WRITE_STREAM)
		, m_compressor(aCompressor)
	{
		if(m_compressor)
		{
			m_compressor->SetOutputCallback([&](
				const void* aBuffer,
				size_t		aBufferSize)
			{				
				File::Writer writer;
				m_file.GetWriter(writer);
				if(writer.Write(aBuffer, aBufferSize) != aBufferSize)
					JELLY_FATAL_ERROR("Failed to write compressed data to file: %s", m_file.GetPath());
			});
		}
	}

	WALWriter::~WALWriter()
	{
	}

	bool
	WALWriter::IsValid() const
	{
		return m_file.IsValid();
	}

	//--------------------------------------------------------------------------

	size_t
	WALWriter::GetSize() 
	{
		return m_file.GetSize();
	}

	void
	WALWriter::WriteItem(
		const IItem*		aItem,
		CompletionEvent*	aCompletionEvent) 
	{
		m_pendingItemWrites.push_back(std::make_pair(aItem, aCompletionEvent));
	}

	void
	WALWriter::Flush() 
	{
		File::Writer uncompressedWriter;
		IWriter* writer;
			
		if(m_compressor)
		{
			writer = m_compressor.get();
		}
		else
		{
			m_file.GetWriter(uncompressedWriter);
			writer = &uncompressedWriter;
		}

		for(std::pair<const IItem*, CompletionEvent*> i : m_pendingItemWrites)					
			i.first->Write(writer, NULL);

		if(m_compressor)
			m_compressor->Flush();

		if(!m_file.Flush())
			JELLY_ASSERT(false);

		for (std::pair<const IItem*, CompletionEvent*> i : m_pendingItemWrites)
			i.second->Signal();

		m_pendingItemWrites.clear();
	}

}