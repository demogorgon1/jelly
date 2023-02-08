#pragma once

// File I/O on Windows

#if defined(_WIN32)

#include <jelly/ErrorUtils.h>
#include <jelly/IReader.h>
#include <jelly/IWriter.h>

namespace jelly
{

	namespace IOWin32
	{

		class Handle
		{
		public:
						Handle(
							HANDLE				aHandle = INVALID_HANDLE_VALUE);
						~Handle();

			Handle&		operator=(
							HANDLE				aHandle);
			void		Release();
			bool		IsSet() const;
			
			operator HANDLE() const
			{
				JELLY_ASSERT(m_handle != INVALID_HANDLE_VALUE);
				return m_handle;
			}

		private:

			HANDLE								m_handle;
		};

		//-----------------------------------------------------------------------------------

		class FileLock
		{
		public:
						FileLock(
							const char*			aPath);
						~FileLock();

		private:

			Handle								m_handle;
			std::string							m_path;
		};

		//-----------------------------------------------------------------------------------

		class FileReadRandom
		{
		public:
						FileReadRandom(
							const char*			aPath);
						~FileReadRandom();

			bool		IsValid();
			void		ReadAtOffset(
							size_t				aOffset,
							void*				aBuffer,
							size_t				aBufferSize);

		private:

			Handle								m_handle;
		};

		//-----------------------------------------------------------------------------------

		class FileReadStream
			: public IReader
		{
		public:
						FileReadStream(
							const char*			aPath);
						~FileReadStream();

			bool		IsValid();
			size_t		GetSize() const;

			// IReader implementation
			size_t		Read(
							void*				aBuffer,
							size_t				aBufferSize) override;
			size_t		GetTotalBytesRead() const override;

		private:

			struct FileReadBuffer
			{
				static const size_t SIZE = 128 * 1024;

				FileReadBuffer()
					: m_readOffset(0)
					, m_bytes(0)
				{

				}

				// Public data
				size_t							m_bytes;
				size_t							m_readOffset;
				uint8_t							m_buffer[SIZE];
			};

			std::unique_ptr<FileReadBuffer>		m_pendingReadBuffer;

			Handle								m_handle;
			size_t								m_size;
			size_t								m_totalBytesRead;

			FileReadBuffer*	_ReadBuffer();
		};

		//-----------------------------------------------------------------------------------

		class FileWriteStream
			: public IWriter
		{
		public:
						FileWriteStream(
							const char*			aPath);
						~FileWriteStream();

			size_t		Flush();

			// IWriter implementation
			size_t		Write(
							const void*			aBuffer,
							size_t				aBufferSize) override;
			size_t		GetTotalBytesWritten() const override;

		private:

			struct FileWriteBuffer
			{
				static const size_t SIZE = 512 * 1024;

				FileWriteBuffer()
					: m_bytes(0)
					, m_spaceLeft(SIZE)
				{

				}

				// Public data
				size_t							m_bytes;
				size_t							m_spaceLeft;
				uint8_t							m_buffer[SIZE];
			};
			
			std::unique_ptr<FileWriteBuffer>	m_pendingWriteBuffer;

			Handle								m_handle;
			size_t								m_size;
			size_t								m_nonFlushedBytes;

			void		_WriteBuffer(
							FileWriteBuffer*	aWriteBuffer);
		};

	}

}

#endif