#pragma once

namespace jelly
{

	class IItem;

	// Interface for store writer implementation
	class IStoreWriter
	{
	public:
		virtual			~IStoreWriter() {}

		// Virtual interface
		virtual size_t	WriteItem(
							const IItem*					aItem) = 0;	
		virtual void	Flush() = 0;
	};

}
