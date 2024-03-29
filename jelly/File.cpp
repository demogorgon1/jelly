#include <jelly/Base.h>

#if defined(JELLY_FORCE_POSIX_FILE_IO)
#define JELLY_POSIX_FILE_IO
#elif defined(JELLY_FORCE_WINDOWS_FILE_IO)
#define JELLY_WINDOWS_FILE_IO
#elif defined(_WIN32)
#define JELLY_WINDOWS_FILE_IO
#else
#define JELLY_POSIX_FILE_IO
#endif

#if defined(JELLY_WINDOWS_FILE_IO)
	#include <windows.h>

	#include "IOWin32.h"

	using namespace jelly::IOWin32;
#elif defined(JELLY_POSIX_FILE_IO)
	#include "IOLinux.h"

	using namespace jelly::IOLinux;
#endif

#include <jelly/ErrorUtils.h>
#include <jelly/File.h>

namespace jelly
{

	struct File::Internal
	{
		Internal(
			const char*			aPath,
			Mode				aMode,
			const FileHeader&	aHeader)
			: m_mode(aMode)
		{
			switch(m_mode)
			{
			case MODE_READ_STREAM:			m_fileReadStream = std::make_unique<FileReadStream>(aPath, aHeader); break;
			case MODE_READ_RANDOM:			m_fileReadRandom = std::make_unique<FileReadRandom>(aPath, aHeader); break;
			case MODE_WRITE_STREAM:			m_fileWriteStream = std::make_unique<FileWriteStream>(aPath, aHeader); break;
			case MODE_MUTEX:				m_fileLock = std::make_unique<FileLock>(aPath); break;
			default:						JELLY_ASSERT(false);
			}
		}

		~Internal()
		{
		}

		// Public data
		Mode								m_mode;

		std::unique_ptr<FileWriteStream>	m_fileWriteStream;
		std::unique_ptr<FileReadStream>		m_fileReadStream;
		std::unique_ptr<FileReadRandom>		m_fileReadRandom;
		std::unique_ptr<FileLock>			m_fileLock;
	};

	//---------------------------------------------------------------------
		
	File::File(
		FileStatsContext*	aStatsContext,
		const char*			aPath,
		Mode				aMode, 
		const FileHeader&	aHeader)
		: m_path(aPath)
		, m_statsContext(aStatsContext)
	{
		m_internal = new Internal(aPath, aMode, aHeader);
	}
		
	File::~File()
	{
		Close();
	}

	void		
	File::Close()
	{
		if(m_internal != NULL)
		{
			delete m_internal;
			m_internal = NULL;
		}
	}

	bool	
	File::IsValid() const noexcept
	{
		if(m_internal == NULL)
			return false;

		if(m_internal->m_fileWriteStream)
			return true;
		else if(m_internal->m_fileReadStream)
			return m_internal->m_fileReadStream->IsValid();
		else if(m_internal->m_fileReadRandom)
			return m_internal->m_fileReadRandom->IsValid();

		JELLY_ASSERT(false);
		return false;
	}

	size_t	
	File::GetSize() const 
	{
		JELLY_ASSERT(m_internal != NULL);
		if(m_internal->m_fileWriteStream)
			return m_internal->m_fileWriteStream->GetTotalBytesWritten();
		if (m_internal->m_fileReadStream)
			return m_internal->m_fileReadStream->GetSize();

		JELLY_ASSERT(false);
		return 0;
	}

	size_t
	File::Flush()
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_WRITE_STREAM);
		JELLY_ASSERT(m_internal->m_fileWriteStream);

		return m_internal->m_fileWriteStream->Flush();
	}

	void	
	File::ReadAtOffset(
		size_t			aOffset,
		void*			aBuffer,
		size_t			aBufferSize)
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_RANDOM);
		JELLY_ASSERT(m_internal->m_fileReadRandom);

		m_internal->m_fileReadRandom->ReadAtOffset(aOffset, aBuffer, aBufferSize);

		if (m_statsContext != NULL && m_statsContext->m_idRead != UINT32_MAX)
			m_statsContext->m_stats->Emit(m_statsContext->m_idRead, aBufferSize, Stat::TYPE_COUNTER);
	}

	size_t		
	File::GetReadOffset() const
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_STREAM);
		JELLY_ASSERT(m_internal->m_fileReadStream);

		return m_internal->m_fileReadStream->GetTotalBytesRead();
	}

	bool		
	File::IsEnd() const noexcept
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_STREAM);
		JELLY_ASSERT(m_internal->m_fileReadStream);

		return m_internal->m_fileReadStream->IsEnd();
	}

	//---------------------------------------------------------------------

	void
	File::Write(
		const void*		aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_WRITE_STREAM);
		JELLY_ASSERT(m_internal->m_fileWriteStream);

		m_internal->m_fileWriteStream->Write(aBuffer, aBufferSize);

		if(m_statsContext != NULL && m_statsContext->m_idWrite != UINT32_MAX)
			m_statsContext->m_stats->Emit(m_statsContext->m_idWrite, aBufferSize, Stat::TYPE_COUNTER);
	}

	size_t		
	File::GetTotalBytesWritten() const 
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_WRITE_STREAM);
		JELLY_ASSERT(m_internal->m_fileWriteStream);

		return m_internal->m_fileWriteStream->GetTotalBytesWritten();
	}

	//---------------------------------------------------------------------

	size_t		
	File::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_STREAM);
		JELLY_ASSERT(m_internal->m_fileReadStream);

		size_t bytes = m_internal->m_fileReadStream->Read(aBuffer, aBufferSize);

		if (m_statsContext != NULL && m_statsContext->m_idRead != UINT32_MAX)
			m_statsContext->m_stats->Emit(m_statsContext->m_idRead, bytes, Stat::TYPE_COUNTER);

		return bytes;
	}

	size_t		
	File::GetTotalBytesRead() const 
	{
		JELLY_ASSERT(m_internal != NULL);
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_STREAM);
		JELLY_ASSERT(m_internal->m_fileReadStream);

		return m_internal->m_fileReadStream->GetTotalBytesRead();
	}

}