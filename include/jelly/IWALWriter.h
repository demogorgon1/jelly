#pragma once

#include "RequestResult.h"

namespace jelly
{

	struct CompletionEvent;
	class ItemBase;
	class ReplicationNetwork;

	// Interface for WAL writer implementation
	class IWALWriter
	{
	public:		
		virtual			~IWALWriter() {}

		// Virtual interface
		virtual size_t	GetSize() const = 0;
		virtual void	WriteItem(
							const ItemBase*		aItem,
							CompletionEvent*	aCompletionEvent,
							RequestResult*		aResult,
							Exception::Code*	aException) = 0;
		virtual size_t	Flush(
							ReplicationNetwork*	aReplicationNetwork) = 0;
		virtual void	Cancel() = 0;
		virtual size_t	GetPendingWriteCount() const = 0;
		virtual bool	HadFailure() const = 0;
	
	};

}
