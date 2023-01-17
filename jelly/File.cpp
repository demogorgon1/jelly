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

#include "File.h"

namespace jelly
{

	struct File::Internal
	{
		Internal(
			const char*			aPath,
			Mode				aMode)
			: m_mode(aMode)
		{
			switch(m_mode)
			{
			case MODE_READ_STREAM:			m_fileReadStream = std::make_unique<FileReadStream>(aPath); break;
			case MODE_READ_RANDOM:			m_fileReadRandom = std::make_unique<FileReadRandom>(aPath); break;
			case MODE_WRITE_STREAM:			m_fileWriteStream = std::make_unique<FileWriteStream>(aPath); break;
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
	};

	//---------------------------------------------------------------------
		
	File::File(
		const char*		aPath,
		Mode			aMode)
		: m_path(aPath)
	{
		m_internal = new Internal(aPath, aMode);
	}
		
	File::~File()
	{
		delete m_internal;
	}

	bool	
	File::IsValid() const
	{
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
		if(m_internal->m_fileWriteStream)
			return m_internal->m_fileWriteStream->GetSize();
		if (m_internal->m_fileReadStream)
			return m_internal->m_fileReadStream->GetSize();

		JELLY_ASSERT(false);
		return 0;
	}

	void
	File::Flush()
	{
		JELLY_ASSERT(m_internal->m_mode == MODE_WRITE_STREAM);
		JELLY_ASSERT(m_internal->m_fileWriteStream);
		m_internal->m_fileWriteStream->Flush();
	}

	void	
	File::ReadAtOffset(
		size_t			aOffset,
		void*			aBuffer,
		size_t			aBufferSize)
	{
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_RANDOM);
		JELLY_ASSERT(m_internal->m_fileReadRandom);
		m_internal->m_fileReadRandom->ReadAtOffset(aOffset, aBuffer, aBufferSize);
	}

	//---------------------------------------------------------------------

	size_t		
	File::Write(
		const void*		aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal->m_mode == MODE_WRITE_STREAM);
		JELLY_ASSERT(m_internal->m_fileWriteStream);
		return m_internal->m_fileWriteStream->Write(aBuffer, aBufferSize);
	}

	//---------------------------------------------------------------------

	size_t		
	File::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal->m_mode == MODE_READ_STREAM);
		JELLY_ASSERT(m_internal->m_fileReadStream);
		return m_internal->m_fileReadStream->Read(aBuffer, aBufferSize);
	}

}