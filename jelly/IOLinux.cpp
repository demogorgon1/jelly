#include <jelly/Base.h>

#if !defined(_WIN32)

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "IOLinux.h"

namespace jelly
{

	namespace IOLinux
	{

		Handle::Handle(
			int					aFd)
			: m_fd(aFd)
		{

		}

		Handle::~Handle()
		{
			Release();
		}

		Handle&
		Handle::operator=(
			int					aFd)
		{
			JELLY_ASSERT(!IsSet());
			m_fd = aFd;
			return *this;
		}

		void
		Handle::Release()
		{
			if (m_fd != -1)
			{
				int result = close(m_fd);
				JELLY_CHECK(result != -1, "close() failed: %u", m_path.c_str(), errno);

				m_fd = -1;
			}
		}

		bool
		Handle::IsSet() const
		{
			return m_handle != m_fd;
		}

		//-----------------------------------------------------------------------------------

		FileReadRandom::FileReadRandom(
			const char*			aPath)
		{
			int flags = O_RDONLY;
			int mode = 0;

			m_handle = open(aPath, flags, mode);
			if(!m_handle.IsSet())
			{
				int errorCode = errno;
				JELLY_CHECK(errorCode == ENOENT, "open() failed: %u (path: %s)", errorCode, aPath);
			}
		}
		
		FileReadRandom::~FileReadRandom()
		{

		}

		bool		
		FileReadRandom::IsValid()
		{
			return m_handle.IsSet();
		}

		void		
		FileReadRandom::ReadAtOffset(
			size_t				aOffset,
			void*				aBuffer,
			size_t				aBufferSize)
		{
			JELLY_ASSERT(m_handle.IsSet());

			{
				off_t result = lseek(m_handle, (off_t)aOffset, SEEK_SET);
				JELLY_CHECK(result == (off_t)aOffset, "lseek() failed (offset %u)", (uint32_t)aOffset);
			}

			{
				ssize_t bytes = read(m_handle, aBuffer, aBufferSize);
				JELLY_CHECK((size_t)bytes == aBufferSize, "read() failed (offset %u, buffer size %u)", (uint32_t)aOffset, (uint32_t)aBufferSize);
			}
		}

		//-----------------------------------------------------------------------------------

		FileReadStream::FileReadStream(
			const char*			aPath)
			: m_size(0)
		{
			int flags = O_RDONLY;
			int mode = 0;

			m_handle = open(aPath, flags, mode);
			if (!m_handle.IsSet())
			{
				int errorCode = errno;
				JELLY_CHECK(errorCode == ENOENT, "open() failed: %u (path: %s)", errorCode, aPath);
			}
			else
			{
				struct stat s;
				int result = fstat(m_handle, &s);
				if (result == -1)
					m_handle.Release();
				else
					m_size = (size_t)s.st_size;
			}
		}

		FileReadStream::~FileReadStream()
		{

		}

		bool		
		FileReadStream::IsValid()
		{
			return m_handle.IsSet();
		}

		size_t		
		FileReadStream::GetSize() const
		{
			JELLY_ASSERT(m_handle.IsSet());

			return m_size;
		}

		size_t		
		FileReadStream::Read(
			void*				aBuffer,
			size_t				aBufferSize) 
		{
			JELLY_ASSERT(m_handle.IsSet());

			ssize_t bytes = read(m_handle, aBuffer, aBufferSize);
			JELLY_CHECK(bytes >= 0, "read() failed: %u", errno);

			return (size_t)bytes;
		}

		//-----------------------------------------------------------------------------------

		FileWriteStream::FileWriteStream(
			const char*			aPath)
			: m_size(0)
		{
			int flags = O_CREAT | O_WRONLY;
			int mode = S_IRUSR | S_IWUSR;

			m_handle = open(aPath, flags, mode);
			JELLY_CHECK(m_handle.IsSet(), "open() failed: %u (path: %s)", errorCode, aPath);
		}
		
		FileWriteStream::~FileWriteStream()
		{
			Flush();
		}

		void		
		FileWriteStream::Flush()
		{
			JELLY_ASSERT(m_handle.IsSet());

			if(m_pendingWriteBuffer)
			{
				_WriteBuffer(m_pendingWriteBuffer.get());

				m_pendingWriteBuffer.reset();
			}

			int result = fsync(m_handle);
			JELLY_CHECK(result != -1, "fsync() failed: %u", errno);
		}

		size_t		
		FileWriteStream::GetSize() const
		{
			JELLY_ASSERT(m_handle.IsSet());

			return m_size;
		}

		size_t		
		FileWriteStream::Write(
			const void*			aBuffer,
			size_t				aBufferSize) 
		{
			JELLY_ASSERT(m_handle.IsSet());
			
			size_t remaining = aBufferSize;
			const uint8_t* p = (const uint8_t*)aBuffer;

			while(remaining > 0)
			{
				if(!m_pendingWriteBuffer || m_pendingWriteBuffer->m_spaceLeft == 0)
				{
					if(m_pendingWriteBuffer)
						_WriteBuffer(m_pendingWriteBuffer.get());

					m_pendingWriteBuffer = std::make_unique<FileWriteBuffer>();
				}

				size_t toCopy = std::min<size_t>(remaining, m_pendingWriteBuffer->m_spaceLeft);

				memcpy(m_pendingWriteBuffer->m_buffer + m_pendingWriteBuffer->m_bytes, p, toCopy);

				m_pendingWriteBuffer->m_bytes += toCopy;
				m_pendingWriteBuffer->m_spaceLeft -= toCopy;
				p += toCopy;
				remaining -= toCopy;
			}

			m_size += aBufferSize;

			return aBufferSize;
		}

		void		
		FileWriteStream::_WriteBuffer(
			FileWriteBuffer*	aWriteBuffer)
		{
			ssize_t bytes = write(m_handle, aWriteBuffer->m_buffer, aWriteBuffer->m_bytes);
			JELLY_CHECK((size_t)bytes == aWriteBuffer->m_bytes, "write() failed");
		}

	}

}

#endif
