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
#elif defined(JELLY_POSIX_FILE_IO)
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#include <stdint.h>

#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>

#include "File.h"

namespace jelly
{

	struct File::Internal
	{
		Internal()
			: m_size(0)
		{
			#if defined(JELLY_WINDOWS_FILE_IO)				
				m_handle = INVALID_HANDLE_VALUE;
			#elif defined(JELLY_POSIX_FILE_IO)
				m_fd = -1;
			#endif
		}

		size_t		m_size;

		#if defined(JELLY_WINDOWS_FILE_IO)				
			HANDLE	m_handle;
		#elif defined(JELLY_POSIX_FILE_IO)
			int		m_fd;
		#endif
	};

	//---------------------------------------------------------------------

	size_t	
	File::Reader::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		#if defined(JELLY_WINDOWS_FILE_IO)		
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

			DWORD bytes;
			if (!ReadFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
				return 0;

			return (size_t)bytes;
		#elif defined(JELLY_POSIX_FILE_IO)
			JELLY_ASSERT(m_internal->m_fd != -1);

			ssize_t bytes = read(m_internal->m_fd, aBuffer, aBufferSize);
			if(bytes < 0)
				return 0;

			return (size_t)bytes;
		#endif
	}

	//---------------------------------------------------------------------

	size_t	
	File::Writer::Write(
		const void*		aBuffer,
		size_t			aBufferSize) 
	{
		#if defined(JELLY_WINDOWS_FILE_IO)		
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

			DWORD bytes;
			if (!WriteFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
				return 0;

			m_internal->m_size += (size_t)bytes;

			return (size_t)bytes;
		#elif defined(JELLY_POSIX_FILE_IO)
			JELLY_ASSERT(m_internal->m_fd != -1);

			ssize_t bytes = write(m_internal->m_fd, aBuffer, aBufferSize);
			if(bytes < 0)
				return 0;

			m_internal->m_size += (size_t)bytes;

			return (size_t)bytes;
		#endif
	}

	//---------------------------------------------------------------------
		
	File::File(
		const char*		aPath,
		Mode			aMode)
		: m_mode(aMode)
		, m_path(aPath)
	{
		m_internal = new Internal();

		#if defined(JELLY_WINDOWS_FILE_IO)		
			
			DWORD desiredAccess = 0; 
			DWORD shareMode = 0;
			DWORD creationDisposition = 0;
			DWORD flags = 0;

			switch(m_mode)
			{
			case MODE_WRITE_STREAM:
				desiredAccess = GENERIC_WRITE;
				creationDisposition = CREATE_ALWAYS;
				flags = FILE_FLAG_SEQUENTIAL_SCAN;
				break;

			case MODE_READ_STREAM:
				desiredAccess = GENERIC_READ;
				creationDisposition = OPEN_EXISTING;
				flags = FILE_FLAG_SEQUENTIAL_SCAN;
				break;

			case MODE_READ_RANDOM:
				desiredAccess = GENERIC_READ;
				creationDisposition = OPEN_EXISTING;
				flags = FILE_FLAG_RANDOM_ACCESS;
				break;

			default:
				JELLY_ASSERT(false);
			}
				
			m_internal->m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);

			if (m_internal->m_handle != INVALID_HANDLE_VALUE && m_mode != MODE_WRITE_STREAM)
			{
				static_assert(sizeof(LARGE_INTEGER) == sizeof(uint64_t));
				uint64_t fileSize;
				if (!GetFileSizeEx(m_internal->m_handle, (LARGE_INTEGER*)&fileSize))
				{
					if(!CloseHandle(m_internal->m_handle))
						JELLY_FATAL_ERROR("CloseHandle() failed (path %s): %u", m_path.c_str(), GetLastError());

					m_internal->m_handle = INVALID_HANDLE_VALUE;
				}
				else
				{
					m_internal->m_size = (size_t)fileSize;
				}
			}

		#elif defined(JELLY_POSIX_FILE_IO)

			int flags = 0;
			int mode = 0;

			switch(m_mode)
			{
			case MODE_WRITE_STREAM:
				flags = O_CREAT | O_WRONLY;
				mode = S_IRUSR | S_IWUSR;
				break;

			case MODE_READ_STREAM:
				flags = O_RDONLY;
				break;

			case MODE_READ_RANDOM:
				flags = O_RDONLY;
				break;

			default:
				JELLY_ASSERT(false);
			}

			m_internal->m_fd = open(aPath, flags, mode);

			if(m_internal->m_fd == -1)
				Log::PrintF(Log::LEVEL_WARNING, "open() failed (path %s): %u", m_path.c_str(), errno);

			if(m_internal->m_fd != -1 && m_mode != MODE_WRITE_STREAM)
			{
				struct stat s;
				int result = fstat(m_internal->m_fd, &s);
				if(result == -1)
				{
					int closeResult = close(m_internal->m_fd);
					JELLY_CHECK(closeResult != -1, "close() failed (path %s): %u", m_path.c_str(), errno);

					m_internal->m_fd = -1;
				}
				else
				{
					m_internal->m_size = (size_t)s.st_size;
				}
			}

		#endif
	}
		
	File::~File()
	{
		#if defined(JELLY_WINDOWS_FILE_IO)	
			if(m_internal->m_handle != INVALID_HANDLE_VALUE)
			{
				if(m_mode == MODE_WRITE_STREAM)
					Flush();

				if(!CloseHandle(m_internal->m_handle))
					JELLY_FATAL_ERROR("CloseHandle() failed (path %s): %u", m_path.c_str(), GetLastError());
			}
		#elif defined(JELLY_POSIX_FILE_IO)
			if(m_internal->m_fd != -1)
			{
				if (m_mode == MODE_WRITE_STREAM)
					Flush();

				int result = close(m_internal->m_fd);
				JELLY_CHECK(result != -1, "close() failed (path %s): %u", m_path.c_str(), errno);
			}
		#endif

		delete m_internal;
	}

	bool	
	File::IsValid() const
	{
		#if defined(JELLY_WINDOWS_FILE_IO)	
			return m_internal->m_handle != INVALID_HANDLE_VALUE;
		#elif defined(JELLY_POSIX_FILE_IO)
			return m_internal->m_fd != -1;
		#endif
	}

	void	
	File::GetReader(
		size_t			aOffset,
		Reader&			aOut)
	{
		if(aOffset != UINT64_MAX)
		{
			JELLY_ASSERT(m_mode == MODE_READ_RANDOM);

			#if defined(JELLY_WINDOWS_FILE_IO)	
				JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

				LARGE_INTEGER distance;
				distance.LowPart = (DWORD)(aOffset & 0xFFFFFFFF);
				distance.HighPart = (DWORD)(aOffset >> 32);

				BOOL result = SetFilePointerEx(m_internal->m_handle, distance, NULL, FILE_BEGIN);
				JELLY_CHECK(result != 0, "SetFilePointerEx() failed (offset %llu, path %s)", aOffset, m_path.c_str());
			#elif defined(JELLY_POSIX_FILE_IO)	
				JELLY_ASSERT(m_internal->m_fd != -1);

				off_t result = lseek(m_internal->m_fd, (off_t)aOffset, SEEK_SET);
				JELLY_CHECK(result == (off_t)aOffset, "lseek() failed (offset %llu, path %s)", aOffset, m_path.c_str());
			#endif
		}
		else
		{
			JELLY_ASSERT(m_mode == MODE_READ_STREAM);
		}				

		aOut.m_internal = m_internal;
	}
		
	void	
	File::GetWriter(
		Writer&			aOut)
	{
		JELLY_ASSERT(m_mode == MODE_WRITE_STREAM);

		aOut.m_internal = m_internal;
	}

	size_t	
	File::GetSize() const
	{
		return m_internal->m_size;
	}

	void
	File::Flush()
	{
		#if defined(JELLY_WINDOWS_FILE_IO)	
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);
			JELLY_ASSERT(m_mode == MODE_WRITE_STREAM);

			if (!FlushFileBuffers(m_internal->m_handle))
				JELLY_FATAL_ERROR("FlushFileBuffers() failed (path %s): %u", m_path.c_str(), GetLastError());
		#elif defined(JELLY_POSIX_FILE_IO)	
			JELLY_ASSERT(m_internal->m_fd != -1);

			int result = fsync(m_internal->m_fd);
			JELLY_CHECK(result != -1, "fsync() failed (path %s): %u", m_path.c_str(), errno);
		#endif
	}

	void	
	File::ReadAtOffset(
		size_t			aOffset,
		void*			aBuffer,
		size_t			aBufferSize)
	{
		JELLY_ASSERT(m_mode == MODE_READ_RANDOM);

		#if defined(JELLY_WINDOWS_FILE_IO)	
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

			LARGE_INTEGER distance;
			distance.LowPart = (DWORD)(aOffset & 0xFFFFFFFF);
			distance.HighPart = (DWORD)(aOffset >> 32);

			{
				BOOL result = SetFilePointerEx(m_internal->m_handle, distance, NULL, FILE_BEGIN);
				JELLY_CHECK(result != 0, "SetFilePointerEx() failed (offset %llu, path %s).", aOffset, m_path.c_str());
			}

			{
				DWORD bytesRead;
				BOOL result = ReadFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytesRead, NULL);
				JELLY_CHECK(result != 0, "ReadFile() failed (offset %llu, buffer size %llu, path %s).", aOffset, aBufferSize, m_path.c_str());
				JELLY_ASSERT(bytesRead == (DWORD)aBufferSize);
			}
		#elif defined(JELLY_POSIX_FILE_IO)	
			JELLY_ASSERT(m_internal->m_fd != -1);

			{
				off_t result = lseek(m_internal->m_fd, (off_t)aOffset, SEEK_SET);
				JELLY_CHECK(result == (off_t)aOffset, "lseek() failed (offset %llu, path %s)", aOffset, m_path.c_str());
			}

			{
				ssize_t bytes = read(m_internal->m_fd, aBuffer, aBufferSize);
				JELLY_CHECK((size_t)bytes == aBufferSize, "read() failed (offset %llu, buffer size %llu, path %s).", aOffset, aBufferSize, m_path.c_str());
			}
		#endif
	}

}