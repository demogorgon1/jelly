#pragma once

#include <jelly/IReader.h>
#include <jelly/IStats.h>
#include <jelly/IWriter.h>

namespace jelly
{

	class File
		: public IReader
		, public IWriter
	{
	public:
		struct Internal;

		enum Mode
		{
			MODE_READ_STREAM,
			MODE_READ_RANDOM,
			MODE_WRITE_STREAM
		};

					File(
						const char*			aPath,
						Mode				aMode);
					~File();

		bool		IsValid() const;
		size_t		GetSize() const;
		size_t		Flush();
		void		ReadAtOffset(
						size_t				aOffset,
						void*				aBuffer,
						size_t				aBufferSize);

		// IWriter implementation
		size_t		Write(
						const void*			aBuffer,
						size_t				aBufferSize) override;

		// IReader implementation
		size_t		Read(
						void*				aBuffer,
						size_t				aBufferSize) override;

		// Data access
		const char*	GetPath() const { return m_path.c_str(); }

	private:

		Internal*		m_internal;
		std::string		m_path;
	};

}