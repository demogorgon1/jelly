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
#elif defined(JELLY_POSIX_FILE_IO)
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
#endif

#include <jelly/ErrorUtils.h>
#include <jelly/Log.h>

#include "File.h"

namespace jelly
{

	namespace
	{
		struct WriteBuffer
		{
			WriteBuffer()
				: m_p(m_data)
				, m_spaceLeft(File::WRITE_BUFFER_SIZE)
				, m_bytes(0)
				, m_next(NULL)
			{

			}

			uint8_t			m_data[File::WRITE_BUFFER_SIZE];
			uint8_t*		m_p;
			size_t			m_spaceLeft;
			size_t			m_bytes;
			WriteBuffer*	m_next;
		};

		struct WriteBufferList
		{
			WriteBufferList()
				: m_head(NULL)
				, m_tail(NULL)
				, m_count(0)
			{

			}

			~WriteBufferList()
			{
				while(m_head != NULL)
				{
					JELLY_ASSERT(m_count > 0);
					WriteBuffer* next = m_head->m_next;
					delete m_head;
					m_head = next;
					m_count--;
				}
				JELLY_ASSERT(m_count == 0);
			}

			void
			Write(
				const void*		aBuffer,
				size_t			aBufferSize)
			{
				size_t remaining = aBufferSize;
				const uint8_t* p = (const uint8_t*)aBuffer;

				while(remaining > 0)
				{
					if(m_tail == NULL || m_tail->m_spaceLeft == 0)
					{
						WriteBuffer* newWriteBuffer = new WriteBuffer();
						if(m_tail != NULL)
							m_tail->m_next = newWriteBuffer;
						else
							m_head = newWriteBuffer;

						m_tail = newWriteBuffer;							

						m_count++;
					}

					JELLY_ASSERT(m_tail->m_spaceLeft > 0);
					size_t toCopy = std::min<size_t>(m_tail->m_spaceLeft, remaining);
					memcpy(m_tail->m_p, p, toCopy);
					m_tail->m_p += toCopy;
					m_tail->m_bytes += toCopy;
					m_tail->m_spaceLeft -= toCopy;
					p += toCopy;
					remaining -= toCopy;
				}
			}

			size_t
			GetCount() const
			{
				return m_head != NULL;
			}

			WriteBuffer*
			DetachNextWriteBuffer()
			{
				if(m_head == NULL)
					return NULL;

				WriteBuffer* nextWriteBuffer = m_head;

				m_head = m_head->m_next;
				
				if(m_head == NULL)
					m_tail = NULL;

				JELLY_ASSERT(m_count > 0);
				m_count--;

				return nextWriteBuffer;
			}

			// Public data
			WriteBuffer*	m_head;
			WriteBuffer*	m_tail;
			size_t			m_count;
		};
	}

	//---------------------------------------------------------------------

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

		size_t
		Write(
			const void*		aBuffer,
			size_t			aBufferSize)
		{
			#if defined(JELLY_WINDOWS_FILE_IO)		
				JELLY_ASSERT(m_handle != INVALID_HANDLE_VALUE);

				DWORD bytes;
				if (!WriteFile(m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
					return 0;

				return (size_t)bytes;
			#elif defined(JELLY_POSIX_FILE_IO)
				JELLY_ASSERT(m_fd != -1);

				ssize_t bytes = write(m_fd, aBuffer, aBufferSize);
				if(bytes < 0)
					return 0;

				return (size_t) bytes;
			#endif
		}


		size_t	
		Read(
			void*			aBuffer,
			size_t			aBufferSize) 
		{
			#if defined(JELLY_WINDOWS_FILE_IO)		
				JELLY_ASSERT(m_handle != INVALID_HANDLE_VALUE);

				DWORD bytes;
				if (!ReadFile(m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
					return 0;

				return (size_t)bytes;
			#elif defined(JELLY_POSIX_FILE_IO)
				JELLY_ASSERT(m_fd != -1);

				ssize_t bytes = read(m_fd, aBuffer, aBufferSize);
				if(bytes < 0)
					return 0;

				return (size_t)bytes;
			#endif
		}

		// Public data
		size_t			m_size;
		WriteBufferList	m_writeBufferList;

		#if defined(JELLY_WINDOWS_FILE_IO)				
			HANDLE		m_handle;
		#elif defined(JELLY_POSIX_FILE_IO)
			int			m_fd;
		#endif
	};

	//---------------------------------------------------------------------

	size_t	
	File::Reader::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		return m_internal->Read(aBuffer, aBufferSize);
	}

	//---------------------------------------------------------------------

	size_t	
	File::Writer::Write(
		const void*		aBuffer,
		size_t			aBufferSize) 
	{
		m_internal->m_writeBufferList.Write(aBuffer, aBufferSize);

		m_internal->m_size += aBufferSize;

		if(m_internal->m_writeBufferList.GetCount() > 1)
		{
			std::unique_ptr<WriteBuffer> writeBuffer(m_internal->m_writeBufferList.DetachNextWriteBuffer());
			JELLY_ASSERT(writeBuffer);

			// FIXME: this could be done asynchronously 
			size_t bytes = m_internal->Write(writeBuffer->m_data, writeBuffer->m_bytes);
			JELLY_CHECK(bytes == writeBuffer->m_bytes, "Unable to write to file.");
		}

		return aBufferSize;
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
		JELLY_ASSERT(m_mode == MODE_WRITE_STREAM);

		if(m_internal->m_writeBufferList.GetCount() > 0)
		{
			for(;;)
			{
				std::unique_ptr<WriteBuffer> writeBuffer(m_internal->m_writeBufferList.DetachNextWriteBuffer());
				if(!writeBuffer)
					break;

				size_t bytes = m_internal->Write(writeBuffer->m_data, writeBuffer->m_bytes);
				JELLY_CHECK(bytes == writeBuffer->m_bytes, "Unable to flush write buffers to file.");
			}

			JELLY_ASSERT(m_internal->m_writeBufferList.GetCount() == 0);
		}

		#if defined(JELLY_WINDOWS_FILE_IO)	
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

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