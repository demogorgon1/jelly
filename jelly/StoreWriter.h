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
						const char*						aPath);
		virtual		~StoreWriter();

		bool		IsValid() const;
			
		// IStoreWriter implementation
		size_t		WriteItem(
						const IItem*					aItem) override;

	private:

		File		m_file;
	};

}