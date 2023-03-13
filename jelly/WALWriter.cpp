#include <jelly/Base.h>

#include <jelly/Completion.h>
#include <jelly/CompletionEvent.h>
#include <jelly/Compression.h>
#include <jelly/ErrorUtils.h>
#include <jelly/ItemBase.h>
#include <jelly/ReplicationNetwork.h>
#include <jelly/Stat.h>
#include <jelly/Stream.h>

#include "WALWriter.h"

namespace jelly
{

	WALWriter::WALWriter(
		const char*						aPath,
		const Compression::IProvider*	aCompression,
		FileStatsContext*				aFileStatsContext,
		const FileHeader&				aFileHeader)
		: m_file(aFileStatsContext, aPath, File::MODE_WRITE_STREAM, aFileHeader)
		, m_compression(aCompression)
		, m_hadFailure(false)
	{
		if(aCompression != NULL)
		{
			m_compressor.reset(aCompression->CreateStreamCompressor());

			m_compressor->SetOutputCallback([&](
				const void* aBuffer,
				size_t		aBufferSize)
			{				
				m_file.Write(aBuffer, aBufferSize);
			});
		}
	}

	WALWriter::~WALWriter()
	{
	}

	bool
	WALWriter::IsValid() const noexcept
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
		Completion*						aCompletion) 
	{
		JELLY_ASSERT(!m_hadFailure);

		m_pendingItemWrites.push_back({ aItem, aCompletion });
	}

	size_t
	WALWriter::Flush(
		ReplicationNetwork*				aReplicationNetwork) 
	{
		JELLY_ASSERT(!m_hadFailure);

		IWriter* writer;
			
		if(m_compressor)
			writer = m_compressor.get();
		else
			writer = &m_file;

		try
		{
			for(PendingItemWrite& t : m_pendingItemWrites)					
				t.m_item->Write(writer);

			if(m_compressor)
				m_compressor->Flush();

			m_file.Flush();
		}
		catch(Exception::Code e)
		{
			// Something went wrong while writing or flushing, fail all requests
			for (PendingItemWrite& t : m_pendingItemWrites)
			{
				if(t.m_completion != NULL)
				{	
					t.m_completion->m_result = REQUEST_RESULT_EXCEPTION;
					t.m_completion->m_exception = e;

					t.m_completion->Signal();
				}
			}

			m_pendingItemWrites.clear();

			m_hadFailure = true;
			return 0;
		}

		// Writing completed successfully
		for (PendingItemWrite& t : m_pendingItemWrites)
		{
			if(t.m_completion != NULL)
				t.m_completion->Signal();
		}

		if(aReplicationNetwork != NULL && aReplicationNetwork->IsLocalNodeMaster())
		{
			Stream::Writer stream(m_compression);

			for (PendingItemWrite& t : m_pendingItemWrites)
				t.m_item->Write(&stream);

			aReplicationNetwork->Send(stream);
		}

		size_t count = m_pendingItemWrites.size();
		m_pendingItemWrites.clear();
		return count;
	}

	void		
	WALWriter::Cancel() noexcept
	{
		for (PendingItemWrite& t : m_pendingItemWrites)
		{
			if(t.m_completion != NULL)
				t.m_completion->OnCancel();
		}

		m_pendingItemWrites.clear();
	}

	size_t		
	WALWriter::GetPendingWriteCount() const
	{
		return m_pendingItemWrites.size();
	}

	bool		
	WALWriter::HadFailure() const 
	{
		return m_hadFailure;
	}

}