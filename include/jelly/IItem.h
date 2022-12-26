#pragma once

namespace jelly
{

	namespace Compression
	{
		class IProvider;
	}

	class IReader;
	class IWriter;

	// Base class for LockNode and BlobNode items
	class IItem
	{
	public:
		enum ReadType
		{
			READ_TYPE_ALL,
			READ_TYPE_BLOB_ONLY
		};

		virtual			~IItem() {}

		// Virtual interface
		virtual bool	Write(
							IWriter*						aWriter,
							const Compression::IProvider*	aItemCompression) const = 0;
		virtual bool	Read(
							IReader*						aReader,
							const Compression::IProvider*	aItemCompression,
							ReadType						aReadType = READ_TYPE_ALL) = 0;
		virtual size_t	GetStoredBlobSize() const = 0;

	};

}