#pragma once

#if !defined(_WIN32)

#include <jelly/ErrorUtils.h>
#include <jelly/IReader.h>
#include <jelly/IWriter.h>

namespace jelly
{

	namespace IOLinux
	{

		class Handle
		{
		public:
						Handle(
							int					aFd = -1);
						~Handle();

			Handle&		operator=(
							int					aFd);
			void		Release();
			bool		IsSet() const;
			
			operator int() const
			{
				JELLY_ASSERT(m_fd != -1);
				return m_fd;
			}

		private:

			int									m_fd;
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
		};

		//-----------------------------------------------------------------------------------

		class FileWriteStream
			: public IWriter
		{
		public:
						FileWriteStream(
							const char*			aPath);
						~FileWriteStream();

			void		Flush();
			size_t		GetSize() const;

			// IWriter implementation
			size_t		Write(
							const void*			aBuffer,
							size_t				aBufferSize) override;

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

			void		_WriteBuffer(
							FileWriteBuffer*	 aWriteBuffer);
		};

	}

}

#endif