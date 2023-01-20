#pragma once

#include <jelly/IItem.h>
#include <jelly/IStoreBlobReader.h>

#include "File.h"

namespace jelly
{

	class StoreBlobReader
		: public IStoreBlobReader
	{
	public:
					StoreBlobReader(
						const char*			aPath,
						FileStatsContext*	aFileStatsContext);
		virtual		~StoreBlobReader();

		bool		IsValid() const;

		// IStoreBlobReader
		void		ReadItemBlob(
						size_t				aOffset, 
						IItem*				aItem) override;
		void		Close() override;

	private:

		std::string					m_path;
		std::unique_ptr<File>		m_file;	
		FileStatsContext*			m_fileStatsContext;
	};

}