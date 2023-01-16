#pragma once

namespace jelly
{

	class IBuffer
	{
	public:
		virtual				~IBuffer() {}

		// Virtual interface
		virtual void		Reset() = 0;
		virtual void		SetSize(
								size_t	aSize) = 0;
		virtual size_t		GetSize() const = 0;
		virtual const void*	GetPointer() const = 0;
		virtual void*		GetPointer() = 0;
	};

}