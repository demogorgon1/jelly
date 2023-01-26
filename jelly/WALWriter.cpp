#include <jelly/Base.h>

#include <jelly/CompletionEvent.h>
#include <jelly/Compression.h>
#include <jelly/ErrorUtils.h>
#include <jelly/ItemBase.h>
#include <jelly/Stat.h>

#include "WALWriter.h"

namespace jelly
{

	WALWriter::WALWriter(
		const char*						aPath,
		Compression::IStreamCompressor* aCompressor,
		FileStatsContext*				aFileStatsContext)
		: m_file(aFileStatsContext, aPath, File::MODE_WRITE_STREAM)
		, m_compressor(aCompressor)
	{
		if(m_compressor)
		{
			m_compressor->SetOutputCallback([&](
				const void* aBuffer,
				size_t		aBufferSize)
			{				
				if(m_file.Write(aBuffer, aBufferSize) != aBufferSize)
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
	WALWriter::GetSize() const
	{
		return m_file.GetSize();
	}

	void
	WALWriter::WriteItem(
		const ItemBase*					aItem,
		CompletionEvent*				aCompletionEvent,
		Result*							aResult) 
	{
		m_pendingItemWrites.push_back({aItem, aCompletionEvent, aResult});
	}

	size_t
	WALWriter::Flush() 
	{
		IWriter* writer;
			
		if(m_compressor)
		{
			writer = m_compressor.get();
		}
		else
		{
			writer = &m_file;
		}

		for(PendingItemWrite& t : m_pendingItemWrites)					
			t.m_item->Write(writer);

		if(m_compressor)
			m_compressor->Flush();

		m_file.Flush();

		for (PendingItemWrite& t : m_pendingItemWrites)
			t.m_completionEvent->Signal();

		size_t count = m_pendingItemWrites.size();
		m_pendingItemWrites.clear();
		return count;
	}

	void		
	WALWriter::Cancel() 
	{
		for (PendingItemWrite& t : m_pendingItemWrites)
		{
			*t.m_result = RESULT_CANCELED;
			t.m_completionEvent->Signal();
		}

		m_pendingItemWrites.clear();
	}

	size_t		
	WALWriter::GetPendingWriteCount() const
	{
		return m_pendingItemWrites.size();
	}

}