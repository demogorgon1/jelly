#pragma once

#include <jelly/IReader.h>
#include <jelly/IWriter.h>

namespace jelly
{

	class File
	{
	public:
		static const size_t WRITE_BUFFER_SIZE = 64 * 1024;

		struct Internal;

		struct Reader
			: public IReader
		{
			// IReader implementation
			size_t	Read(
						void*		aBuffer,
						size_t		aBufferSize) override;

			// Public data
			Internal*	m_internal;
		};

		struct Writer
			: public IWriter
		{
			// IWriter implementation
			size_t	Write(
						const void*	aBuffer,
						size_t		aBufferSize) override;

			// Public data
			Internal*	m_internal;
		};

		enum Mode
		{
			MODE_READ_STREAM,
			MODE_READ_RANDOM,
			MODE_WRITE_STREAM
		};

					File(
						const char*		aPath,
						Mode			aMode);
					~File();

		bool		IsValid() const;
		void		GetReader(
						size_t			aOffset,
						Reader&			aOut);
		void		GetWriter(
						Writer&			aOut);
		size_t		GetSize() const;
		void		Flush();
		void		ReadAtOffset(
						size_t			aOffset,
						void*			aBuffer,
						size_t			aBufferSize);

		// Data access
		const char*	GetPath() const { return m_path.c_str(); }

	private:

		Mode		m_mode;
		Internal*	m_internal;
		std::string	m_path;
	};

}