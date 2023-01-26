#pragma once

#include <jelly/File.h>
#include <jelly/IStoreWriter.h>

namespace jelly
{

	// DefaultHost implementation of IStoreWriter
	class StoreWriter
		: public IStoreWriter
	{
	public:			
					StoreWriter(
						const char*						aPath,
						FileStatsContext*				aFileStatsContext);
		virtual		~StoreWriter();

		bool		IsValid() const;
			
		// IStoreWriter implementation
		size_t		WriteItem(
						const ItemBase*					aItem) override;
		void		Flush() override;

	private:

		File		m_file;
	};

}