#pragma once

namespace jelly
{

	class ItemBase;

	// Interface for random-access store blob reader implementation
	class IStoreBlobReader
	{
	public:
		virtual			~IStoreBlobReader() {}

		// Virtual interface
		virtual void	ReadItemBlob(
							size_t					aOffset,
							ItemBase*				aItem) = 0;
		virtual void	Close() = 0;
	};

}