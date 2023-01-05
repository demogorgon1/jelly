#pragma once

#include <stddef.h>

namespace jelly
{

	namespace Compression
	{
		class IProvider;
	}

	class IItem;

	// Interface for store writer implementation
	class IStoreWriter
	{
	public:
		virtual			~IStoreWriter() {}

		// Virtual interface
		virtual size_t	WriteItem(
							const IItem*					aItem,
							const Compression::IProvider*	aItemCompression) = 0;	
	};

}
