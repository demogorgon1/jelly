#pragma once

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
		virtual size_t	GetSize() = 0;
		virtual void	WriteItem(
							const IItem*		aItem,
							CompletionEvent*	aCompletionEvent) = 0;
		virtual void	Flush() = 0;
	
	};

}
