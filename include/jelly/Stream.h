#pragma once

#include "Compression.h"
#include "IReader.h"
#include "IWriter.h"

namespace jelly
{

	namespace Stream
	{

		struct Buffer
		{
			Buffer()
			{

			}

			~Buffer()
			{
				// Don't delete recursively as it could cause stack overflow for long chains
				Buffer* p = m_next;
				while(p != NULL)
				{
					Buffer* next = p->m_next;
					p->m_next = NULL;
					delete p;
					p = next;
				}
			}

			static const size_t MAX_SIZE = 8192;
			uint8_t		m_data[MAX_SIZE];
			size_t		m_size = 0;
			Buffer*		m_next = NULL;
		};

		class Writer
			: public IWriter
		{
		public:
							Writer(
								const Compression::IProvider*			aCompression);
			virtual			~Writer();

			Buffer*			DetachBuffers();
			void			Append(
								Writer&									aOther);

			// IWriter implementation
			void			Write(
								const void*								aBuffer,
								size_t									aBufferSize) override;
			size_t			GetTotalBytesWritten() const override;

		private:

			std::unique_ptr<Compression::IStreamCompressor>		m_compressor;
			Buffer*												m_head;
			Buffer*												m_tail;
			size_t												m_totalBytesWritten;
		};

		class Reader
			: public IReader
		{
		public:
							Reader(
								const Compression::IProvider*			aCompression,
								const Buffer*							aHead);
			virtual			~Reader();

			void			Append(
								const Buffer*							aHead);
			bool			IsEnd() const;

			// IReader implementation
			size_t			Read(
								void*									aBuffer,
								size_t									aBufferSize) override;
			size_t			GetTotalBytesRead() const override;

		private:

			std::unique_ptr<Compression::IStreamDecompressor>	m_decompressor;
			Buffer*												m_decompressedHead;
			Buffer*												m_decompressedTail;
			std::list<const Buffer*>							m_rawBuffers;
			size_t												m_totalBytesRead;
			size_t												m_headReadOffset;
		};

	}

}