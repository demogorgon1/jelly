#pragma once

namespace jelly
{

	/**
	 * \brief Abstract interface for a binary buffer.
	 * 
	 * @see Buffer
	 */
	class IBuffer
	{
	public:
		virtual				~IBuffer() {}

		//---------------------------------------------------------------------
		// Virtual interface

		//! Resets the buffer: size is set to 0 and any allocated memory is released.
		virtual void		Reset() = 0;

		//! Set size of buffer. Existing data is retained, but if new size is smaller than the previous one, the data is truncated accordingly.
		virtual void		SetSize(
								size_t	aSize) = 0;

		//! Returns the size of the buffer. 
		virtual size_t		GetSize() const = 0;

		//! Get const pointer to data contained in buffer.
		virtual const void*	GetPointer() const = 0;

		//! Get pointer to data contained in buffer.
		virtual void*		GetPointer() = 0;
	};

}