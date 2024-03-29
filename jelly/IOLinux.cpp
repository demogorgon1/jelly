#include <jelly/Base.h>

#if !defined(_WIN32)

#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <jelly/FileHeader.h>

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
				JELLY_ALWAYS_ASSERT(result != -1, "close() failed: %u", errno);

				m_fd = -1;
			}
		}

		bool
		Handle::IsSet() const noexcept
		{
			return m_fd != -1;
		}

		//-----------------------------------------------------------------------------------

		FileLock::FileLock(
			const char*			aPath)
			: m_path(aPath)
		{
			m_handle = open(aPath, O_RDWR | O_CREAT, 0666);
			JELLY_CHECK(m_handle.IsSet(), Exception::ERROR_FILE_LOCK_FAILED_TO_OPEN, "Path=%s;ErrorCode=%d", aPath, errno);

			int result = flock(m_handle, LOCK_EX | LOCK_NB);
			JELLY_CHECK(result == 0, Exception::ERROR_FILE_LOCK_ALREADY_LOCKED, "Path=%s;ErrorCode=%d", aPath, errno);
		}
		
		FileLock::~FileLock()
		{
			flock(m_handle, LOCK_UN);

			m_handle.Release();

			// Try to delete the file, but it could fail if someone else took the lock already
			std::error_code result;
			std::filesystem::remove(m_path.c_str(), result);
		}

		//-----------------------------------------------------------------------------------

		FileReadRandom::FileReadRandom(
			const char*			aPath,
			const FileHeader&	aHeader)
		{
			int flags = O_RDONLY;
			int mode = 0;

			m_handle = open(aPath, flags, mode);
			if(!m_handle.IsSet())
			{
				int errorCode = errno;
				JELLY_CHECK(errorCode == ENOENT, Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%d", aPath, errorCode);
			}
			else
			{
				FileHeader header;
				ssize_t bytes = read(m_handle, &header, sizeof(header));
				JELLY_CHECK((size_t)bytes == sizeof(header), Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_READ_HEADER, "Path=%s;ErrorCode=%d", aPath, errno);
				JELLY_CHECK(header == aHeader, Exception::ERROR_FILE_READ_RANDOM_HEADER_MISMATCH, "Path=%s", aPath);
			}
		}
		
		FileReadRandom::~FileReadRandom()
		{

		}

		bool		
		FileReadRandom::IsValid() noexcept
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

			ssize_t bytes = pread(m_handle, aBuffer, aBufferSize, (off_t)aOffset);
			JELLY_CHECK((size_t)bytes == aBufferSize, Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_READ, "Offset=%zu;BufferSize=%zu", aOffset, aBufferSize);
		}

		//-----------------------------------------------------------------------------------

		FileReadStream::FileReadStream(
			const char*			aPath,
			const FileHeader&	aHeader)
			: m_size(0)
			, m_totalBytesRead(0)
		{
			int flags = O_RDONLY;
			int mode = 0;

			m_handle = open(aPath, flags, mode);
			if (!m_handle.IsSet())
			{
				int errorCode = errno;
				JELLY_CHECK(errorCode == ENOENT, Exception::ERROR_FILE_READ_STREAM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%d", aPath, errorCode);
			}
			else
			{
				struct stat s;
				int result = fstat(m_handle, &s);
				if (result == -1)
					m_handle.Release();
				else
					m_size = (size_t)s.st_size;

				if(m_handle.IsSet() && m_size > 0)
				{
					FileHeader header;
					ssize_t bytes = read(m_handle, &header, sizeof(header));
					JELLY_CHECK((size_t)bytes == sizeof(header), Exception::ERROR_FILE_READ_STREAM_FAILED_TO_READ_HEADER, "Path=%s;ErrorCode=%d", aPath, errno);
					JELLY_CHECK(header == aHeader, Exception::ERROR_FILE_READ_STREAM_HEADER_MISMATCH, "Path=%s", aPath);

					m_totalBytesRead = sizeof(header);
				}
			}
		}

		FileReadStream::~FileReadStream()
		{

		}

		bool		
		FileReadStream::IsValid() noexcept
		{
			return m_handle.IsSet();
		}

		size_t		
		FileReadStream::GetSize() const
		{
			JELLY_ASSERT(m_handle.IsSet());

			return m_size;
		}

		bool			
		FileReadStream::IsEnd() const noexcept
		{
			return m_totalBytesRead == m_size;
		}

		size_t		
		FileReadStream::Read(
			void*				aBuffer,
			size_t				aBufferSize) 
		{
			JELLY_ASSERT(m_handle.IsSet());

			size_t remaining = aBufferSize;
			uint8_t* p = (uint8_t*)aBuffer;
			size_t readBytes = 0;

			while(remaining > 0)
			{
				if(!m_pendingReadBuffer || m_pendingReadBuffer->m_readOffset == m_pendingReadBuffer->m_bytes)
					m_pendingReadBuffer.reset(_ReadBuffer());

				size_t bytesLeftInReadBuffer = m_pendingReadBuffer->m_bytes - m_pendingReadBuffer->m_readOffset;
				if(bytesLeftInReadBuffer == 0)
					break;

				size_t toCopy = std::min<size_t>(remaining, bytesLeftInReadBuffer);

				memcpy(p, m_pendingReadBuffer->m_buffer + m_pendingReadBuffer->m_readOffset, toCopy);

				m_pendingReadBuffer->m_readOffset += toCopy;
				p += toCopy;
				readBytes += toCopy;
				remaining -= toCopy;
			}

			m_totalBytesRead += readBytes;

			return readBytes;
		}

		size_t			
		FileReadStream::GetTotalBytesRead() const 
		{
			return m_totalBytesRead;
		}

		FileReadStream::FileReadBuffer* 
		FileReadStream::_ReadBuffer()
		{
			std::unique_ptr<FileReadBuffer> readBuffer = std::make_unique<FileReadBuffer>();
			
			ssize_t bytes = read(m_handle, readBuffer->m_buffer, FileReadBuffer::SIZE);
			JELLY_CHECK(bytes >= 0, Exception::ERROR_FILE_READ_STREAM_FAILED_TO_READ, "ErrorCode=%d", errno);

			readBuffer->m_bytes = (size_t)bytes;

			return readBuffer.release();
		}

		//-----------------------------------------------------------------------------------

		FileWriteStream::FileWriteStream(
			const char*			aPath,
			const FileHeader&	aHeader)
			: m_size(0)
			, m_nonFlushedBytes(0)
		{
			int flags = O_CREAT | O_WRONLY;
			int mode = S_IRUSR | S_IWUSR;

			m_handle = open(aPath, flags, mode);
			JELLY_CHECK(m_handle.IsSet(), Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%d", aPath, errno);

			Write(&aHeader, sizeof(aHeader));
		}
		
		FileWriteStream::~FileWriteStream()
		{
		}

		size_t		
		FileWriteStream::Flush()
		{
			JELLY_ASSERT(m_handle.IsSet());

			if(m_pendingWriteBuffer)
			{
				_WriteBuffer(m_pendingWriteBuffer.get());

				m_pendingWriteBuffer.reset();
			}

			int result = fsync(m_handle);
			JELLY_CHECK(result != -1, Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_FLUSH, "ErrorCode=%d", errno);

			size_t flushedBytes = m_nonFlushedBytes;
			m_nonFlushedBytes = 0;
			return flushedBytes;
		}

		void
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
		}

		size_t			
		FileWriteStream::GetTotalBytesWritten() const
		{
			JELLY_ASSERT(m_handle.IsSet());

			return m_size;
		}

		void		
		FileWriteStream::_WriteBuffer(
			FileWriteBuffer*	aWriteBuffer)
		{
			ssize_t bytes = write(m_handle, aWriteBuffer->m_buffer, aWriteBuffer->m_bytes);
			JELLY_CHECK((size_t)bytes == aWriteBuffer->m_bytes, Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE, "ErrorCode=%d", errno);

			m_nonFlushedBytes += aWriteBuffer->m_bytes;
		}

	}

}

#endif
