#pragma once

#include "ErrorUtils.h"
#include "IBuffer.h"

namespace jelly
{

	/**
	 * \brief Binary buffer object implementing the IBuffer interface.
	 *
	 * \tparam _StaticSize		Size of static part of the buffer. Memory will only be allocated dynamically if buffer size exceeds this.
	 * 
	 * \see Blob
	 * \see BufferReader
	 * \see BufferWriter
	 */
	template <size_t _StaticSize>
	class Buffer
		: public IBuffer
	{
	public:
		Buffer()
			: m_size(0)
			, m_data(m_static)
			, m_bufferSize(_StaticSize)
		{

		}

		Buffer(
			const IBuffer&	aOther)
			: m_size(0)
			, m_data(m_static)
			, m_bufferSize(_StaticSize)
		{
			_Copy(aOther);
		}

		Buffer(
			const Buffer<_StaticSize>&	aOther)
			: m_size(0)
			, m_data(m_static)
			, m_bufferSize(_StaticSize)
		{
			_Copy(aOther);
		}

		virtual
		~Buffer()
		{
			_Reset();
		}

		//! Returns whether or not buffer is static.
		bool
		IsStatic() const
		{
			return m_data == m_static;
		}

		//! Compares contents with the specified IBuffer object. 
		bool
		operator==(
			const IBuffer&	aOther) const
		{
			if(m_size != aOther.GetSize())
				return false;

			return memcmp(m_data, aOther.GetPointer(), m_size) == 0;
		}

		//! Copies the contents of the specified \ref IBuffer object.
		Buffer<_StaticSize>&
		operator=(
			const IBuffer&	aOther) 
		{
			_Copy(aOther);

			return *this;
		}

		//! Copies the contents of the specified \ref Buffer object.
		Buffer<_StaticSize>&
		operator=(
			const Buffer<_StaticSize>&	aOther)
		{
			_Copy(aOther);

			return *this;
		}

		//------------------------------------------------------------------------------
		// IBuffer implentation
		void		
		Reset() override
		{
			_Reset();
		}

		void		
		SetSize(
			size_t		aSize) override
		{
			_Realloc(aSize);
		}
		
		size_t		
		GetSize() const override
		{
			return m_size;
		}
		
		const void* 
		GetPointer() const override
		{
			return m_data;
		}

		void* 
		GetPointer() override
		{
			return m_data;
		}

	private:

		uint8_t		m_static[_StaticSize];
		uint8_t*	m_data;
		size_t		m_size;
		size_t		m_bufferSize;

		void
		_Copy(
			const IBuffer&	aOther)
		{
			_Realloc(aOther.GetSize());

			memcpy(m_data, aOther.GetPointer(), m_size);
		}

		void
		_Reset()
		{
			if (!IsStatic())
				delete[] m_data;
				
			m_bufferSize = _StaticSize;
			m_data = m_static;
			m_size = 0;
		}

		void	
		_Realloc(
			size_t			aSize)
		{
			if(aSize == m_size)
				return;

			if(aSize <= m_bufferSize)
			{
				m_size = aSize;
			}
			else
			{
				JELLY_ASSERT(aSize > m_size);

				size_t newBufferSize = (1 + (aSize / _StaticSize)) * _StaticSize;
				uint8_t* newData = new uint8_t[newBufferSize];
				
				if(m_size > 0)
				{
					JELLY_ASSERT(m_size <= m_bufferSize);
					JELLY_ASSERT(m_size <= newBufferSize);

					memcpy(newData, m_data, m_size);
				}
				
				delete [] newData;

				m_size = aSize;
				m_bufferSize = newBufferSize;
				m_data = newData;
			}
		}
	};

}