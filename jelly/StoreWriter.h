#pragma once

#include <jelly/IStoreWriter.h>

#include "File.h"

namespace jelly
{

	class StoreWriter
		: public IStoreWriter
	{
	public:			
					StoreWriter(
						IStats*							aStats,
						const char*						aPath);
		virtual		~StoreWriter();

		bool		IsValid() const;
			
		// IStoreWriter implementation
		size_t		WriteItem(
						const IItem*					aItem) override;
		void		Flush() override;

	private:

		IStats*		m_stats;
		File		m_file;
	};

}