#pragma once

namespace jelly
{

	class ItemBase;

	// Interface for store writer implementation
	class IStoreWriter
	{
	public:
		virtual			~IStoreWriter() {}

		// Virtual interface
		virtual size_t	WriteItem(
							const ItemBase*					aItem) = 0;	
		virtual void	Flush() = 0;
	};

}
