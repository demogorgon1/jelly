#include <jelly/Base.h>

#if defined(_WIN32)

#include <windows.h>

#include <jelly/FileHeader.h>

#include "IOWin32.h"

namespace jelly
{

	namespace IOWin32
	{

		Handle::Handle(
			HANDLE				aHandle)
			: m_handle(aHandle)
		{

		}

		Handle::~Handle()
		{
			Release();
		}

		Handle&
		Handle::operator=(
			HANDLE				aHandle)
		{
			JELLY_ASSERT(!IsSet());
			m_handle = aHandle;
			return *this;
		}

		void
		Handle::Release()
		{
			if (m_handle != INVALID_HANDLE_VALUE)
			{
				BOOL result = CloseHandle(m_handle);
				JELLY_ALWAYS_ASSERT(result != 0, "CloseHandle() failed: %u", GetLastError());

				m_handle = INVALID_HANDLE_VALUE;
			}
		}

		bool
		Handle::IsSet() const
		{
			return m_handle != INVALID_HANDLE_VALUE;
		}

		//-----------------------------------------------------------------------------------

		FileLock::FileLock(
			const char*			aPath)
			: m_path(aPath)
		{
			DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE;
			DWORD shareMode = 0;
			DWORD creationDisposition = CREATE_ALWAYS;
			DWORD flags = 0;

			m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);
			if (!m_handle.IsSet())
			{
				DWORD errorCode = GetLastError();

				if(errorCode == ERROR_SHARING_VIOLATION)
					JELLY_FAIL(Exception::ERROR_FILE_LOCK_ALREADY_LOCKED, "Path=%s", aPath);
				else
					JELLY_FAIL(Exception::ERROR_FILE_LOCK_FAILED_TO_OPEN, "Path=%s;ErrorCode=%u", aPath, errorCode);
			}
		}
		
		FileLock::~FileLock()
		{
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
			DWORD desiredAccess = GENERIC_READ;
			DWORD shareMode = FILE_SHARE_READ;
			DWORD creationDisposition = OPEN_EXISTING;
			DWORD flags = FILE_FLAG_RANDOM_ACCESS;

			m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);
			if (!m_handle.IsSet())
			{
				DWORD errorCode = GetLastError();
				JELLY_CHECK(errorCode == ERROR_FILE_NOT_FOUND, Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%u", aPath, errorCode);
			}
			else
			{
				FileHeader header;
				ReadAtOffset(0, &header, sizeof(header));
				JELLY_CHECK(header == aHeader, Exception::ERROR_FILE_READ_RANDOM_HEADER_MISMATCH, "Path=%s", aPath);
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

			OVERLAPPED overlapped;
			memset(&overlapped, 0, sizeof(overlapped));
			overlapped.Offset = (DWORD)(aOffset & UINT32_MAX);
			overlapped.OffsetHigh = (DWORD)((aOffset >> 32) & UINT32_MAX);

			DWORD bytesRead;
			BOOL result = ReadFile(m_handle, aBuffer, (DWORD)aBufferSize, &bytesRead, &overlapped);
			JELLY_CHECK(result != 0, Exception::ERROR_FILE_READ_RANDOM_FAILED_TO_READ, "Offset=%zu;BufferSize=%zu", aOffset, aBufferSize);
			JELLY_ASSERT(bytesRead == (DWORD)aBufferSize);
		}

		//-----------------------------------------------------------------------------------

		FileReadStream::FileReadStream(
			const char*			aPath,
			const FileHeader&	aHeader)
			: m_size(0)
			, m_totalBytesRead(0)
		{
			DWORD desiredAccess = GENERIC_READ;
			DWORD shareMode = FILE_SHARE_READ;
			DWORD creationDisposition = OPEN_EXISTING;
			DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;

			m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);
			if(!m_handle.IsSet())
			{
				DWORD errorCode = GetLastError();
				JELLY_CHECK(errorCode == ERROR_FILE_NOT_FOUND, Exception::ERROR_FILE_READ_STREAM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%u", aPath, errorCode);
			}			
			else
			{
				static_assert(sizeof(LARGE_INTEGER) == sizeof(uint64_t));
				uint64_t fileSize;
				if (GetFileSizeEx(m_handle, (LARGE_INTEGER*)&fileSize) != 0)
					m_size = (size_t)fileSize;
				else
					m_handle.Release();

				if(m_handle.IsSet() && m_size > 0)
				{
					FileHeader header;					
					DWORD bytes;
					BOOL result = ReadFile(m_handle, &header, (DWORD)sizeof(header), &bytes, NULL);
					JELLY_CHECK(result != 0 && bytes == (DWORD)sizeof(header), Exception::ERROR_FILE_READ_STREAM_FAILED_TO_READ_HEADER, "Path=%s;ErrorCode=%u", aPath, GetLastError());
					JELLY_CHECK(header == aHeader, Exception::ERROR_FILE_READ_STREAM_HEADER_MISMATCH, "Path=%s", aPath);

					m_totalBytesRead += sizeof(header);
				}
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

		bool		
		FileReadStream::IsEnd() const
		{
			JELLY_ASSERT(m_handle.IsSet());

			return m_size == m_totalBytesRead;
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
			
			DWORD bytes;
			BOOL result = ReadFile(m_handle, readBuffer->m_buffer, (DWORD)FileReadBuffer::SIZE, &bytes, NULL);
			JELLY_CHECK(result != 0, Exception::ERROR_FILE_READ_STREAM_FAILED_TO_READ, "ErrorCode=%u", GetLastError());

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
			// I've tried different kinds of shenanigans (overlapped io, pre-allocating space with volume management privilege),
			// but nothing seems to be faster than just plain (big) synchronous writes. It's a basic property of NTFS and windows that
			// appending to a file will always be synchronous, so it probably just boils down to that. I haven't tried committing 
			// writes on a different thread, but it would kinda mess up the whole architecture. Might try that at some point.

			DWORD desiredAccess = GENERIC_WRITE;
			DWORD shareMode = 0;
			DWORD creationDisposition = CREATE_ALWAYS;
			DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;

			m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);
			JELLY_CHECK(m_handle.IsSet(), Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_OPEN, "Path=%s;ErrorCode=%u", aPath, GetLastError());

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

			BOOL result = FlushFileBuffers(m_handle);
			JELLY_CHECK(result != 0, Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_FLUSH, "ErrorCode=%u", GetLastError());
			
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
			DWORD bytes;
			BOOL result = WriteFile(m_handle, aWriteBuffer->m_buffer, (DWORD)aWriteBuffer->m_bytes, &bytes, NULL);
			JELLY_CHECK(result != 0 && (size_t)bytes == aWriteBuffer->m_bytes, Exception::ERROR_FILE_WRITE_STREAM_FAILED_TO_WRITE, "ErrorCode=%u", GetLastError());

			m_nonFlushedBytes += aWriteBuffer->m_bytes;
		}

	}

}

#endif
