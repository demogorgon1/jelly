#pragma once

#include <functional>

namespace jelly
{

	class IItem;

	// Interface for random-access store blob reader implementation
	class IStoreBlobReader
	{
	public:
		virtual			~IStoreBlobReader() {}

		// Virtual interface
		virtual void	ReadItemBlob(
							size_t				aOffset,
							IItem*				aItem) = 0;
		virtual void	Close() = 0;
	};

}