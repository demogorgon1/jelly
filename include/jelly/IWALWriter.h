#pragma once

#include "Result.h"

namespace jelly
{

	struct CompletionEvent;
	class IItem;

	// Interface for WAL writer implementation
	class IWALWriter
	{
	public:
		virtual			~IWALWriter() {}

		// Virtual interface
		virtual size_t	GetSize() const = 0;
		virtual void	WriteItem(
							const IItem*		aItem,
							CompletionEvent*	aCompletionEvent,
							Result*				aResult) = 0;
		virtual void	Flush() = 0;
		virtual void	Cancel() = 0;
		virtual size_t	GetPendingWriteCount() const = 0;
	
	};

}
