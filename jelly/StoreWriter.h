#pragma once

#include <jelly/File.h>
#include <jelly/IStoreWriter.h>

namespace jelly
{

	struct FileHeader;

	// DefaultHost implementation of IStoreWriter
	class StoreWriter
		: public IStoreWriter
	{
	public:			
					StoreWriter(
						const char*						aTargetPath,
						const char*						aTempPath,
						FileStatsContext*				aFileStatsContext,
						const FileHeader&				aFileHeader);
		virtual		~StoreWriter();

		bool		IsValid() const noexcept;
			
		// IStoreWriter implementation
		size_t		WriteItem(
						const ItemBase*					aItem) override;
		void		Flush() override;

	private:

		std::string m_tempPath;
		std::string m_targetPath;

		File		m_file;
		bool		m_isFlushed;
	};

}