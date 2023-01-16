#pragma once

#include "BlobBuffer.h"
#include "ErrorUtils.h"

namespace jelly
{

	class IReader;
	class IWriter;

	// Base class for LockNode and BlobNode items
	class IItem
	{
	public:
		virtual			~IItem() {}

		// Virtual interface
		virtual size_t	Write(
							IWriter*						aWriter) const = 0;
		virtual bool	Read(
							IReader*						aReader,
							size_t*							aOutBlobOffset) = 0;

		// Virtual methods
		virtual void	UpdateBlobBuffer(
							std::unique_ptr<BlobBuffer>&	/*aBlobBuffer*/) { JELLY_ASSERT(false); }
		virtual size_t	GetStoredBlobSize() const { JELLY_ASSERT(false); return 0; }
	};

}