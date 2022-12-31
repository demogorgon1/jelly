#if defined(_WIN32)
	#include <windows.h>
#endif

#include <stdint.h>

#include <jelly/ErrorUtils.h>

#include "File.h"

namespace jelly
{

	struct File::Internal
	{
		Internal()
			: m_size(0)
		{
			#if defined(_WIN32)				
				m_handle = INVALID_HANDLE_VALUE;
			#endif
		}

		size_t		m_size;

		#if defined(_WIN32)				
			HANDLE	m_handle;
		#endif
	};

	//---------------------------------------------------------------------

	size_t	
	File::Reader::Read(
		void*			aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

		DWORD bytes;
		if (!ReadFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
			return 0;

		return (size_t)bytes;
	}

	//---------------------------------------------------------------------

	size_t	
	File::Writer::Write(
		const void*		aBuffer,
		size_t			aBufferSize) 
	{
		JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

		DWORD bytes;
		if (!WriteFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytes, NULL))
			return 0;

		m_internal->m_size += (size_t)bytes;

		return (size_t)bytes;
	}

	//---------------------------------------------------------------------
		
	File::File(
		const char*		aPath,
		Mode			aMode)
		: m_mode(aMode)
		, m_path(aPath)
	{
		m_internal = new Internal();

		#if defined(_WIN32)
			
			DWORD desiredAccess = 0; 
			DWORD shareMode = 0;
			DWORD creationDisposition = 0;
			DWORD flags = 0;

			switch(m_mode)
			{
			case MODE_WRITE_STREAM:
				desiredAccess = GENERIC_WRITE;
				creationDisposition = CREATE_ALWAYS;
				flags = FILE_FLAG_SEQUENTIAL_SCAN;
				break;

			case MODE_READ_STREAM:
				desiredAccess = GENERIC_READ;
				creationDisposition = OPEN_EXISTING;
				flags = FILE_FLAG_SEQUENTIAL_SCAN;
				break;

			case MODE_READ_RANDOM:
				desiredAccess = GENERIC_READ;
				creationDisposition = OPEN_EXISTING;
				flags = FILE_FLAG_RANDOM_ACCESS;
				break;

			default:
				JELLY_ASSERT(false);
			}
				
			m_internal->m_handle = CreateFileA(aPath, desiredAccess, shareMode, NULL, creationDisposition, flags, NULL);

			if (m_internal->m_handle != INVALID_HANDLE_VALUE && m_mode != MODE_WRITE_STREAM)
			{
				static_assert(sizeof(LARGE_INTEGER) == sizeof(uint64_t));
				uint64_t fileSize;
				if (!GetFileSizeEx(m_internal->m_handle, (LARGE_INTEGER*)&fileSize))
				{
					CloseHandle(m_internal->m_handle);
					m_internal->m_handle = INVALID_HANDLE_VALUE;
				}
				else
				{
					m_internal->m_size = (size_t)fileSize;
				}
			}
		#endif
	}
		
	File::~File()
	{
		#if defined(_WIN32)
			if(m_internal->m_handle != INVALID_HANDLE_VALUE)
				CloseHandle(m_internal->m_handle);
		#endif

		delete m_internal;
	}

	bool	
	File::IsValid() const
	{
		#if defined(_WIN32)
			return m_internal->m_handle != INVALID_HANDLE_VALUE;
		#endif
	}

	void	
	File::GetReader(
		size_t			aOffset,
		Reader&			aOut)
	{
		if(aOffset != UINT64_MAX)
		{
			JELLY_ASSERT(m_mode == MODE_READ_RANDOM);

			#if defined(_WIN32)
				JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

				LARGE_INTEGER distance;
				distance.LowPart = (DWORD)(aOffset & 0xFFFFFFFF);
				distance.HighPart = (DWORD)(aOffset >> 32);

				BOOL result = SetFilePointerEx(m_internal->m_handle, distance, NULL, FILE_BEGIN);
				JELLY_CHECK(result != 0, "SetFilePointerEx() failed (offset %llu, path %s)", aOffset, m_path.c_str());
			#endif
		}
		else
		{
			JELLY_ASSERT(m_mode == MODE_READ_STREAM);
		}				

		aOut.m_internal = m_internal;
	}
		
	void	
	File::GetWriter(
		Writer&			aOut)
	{
		JELLY_ASSERT(m_mode == MODE_WRITE_STREAM);

		aOut.m_internal = m_internal;
	}

	size_t	
	File::GetSize() const
	{
		return m_internal->m_size;
	}

	bool	
	File::Flush()
	{
		#if defined(_WIN32)
			if (!FlushFileBuffers(m_internal->m_handle))
				return false;
		#endif

		return true;
	}

	void	
	File::ReadAtOffset(
		size_t			aOffset,
		void*			aBuffer,
		size_t			aBufferSize)
	{
		JELLY_ASSERT(m_mode == MODE_READ_RANDOM);

		#if defined(_WIN32)
			JELLY_ASSERT(m_internal->m_handle != INVALID_HANDLE_VALUE);

			LARGE_INTEGER distance;
			distance.LowPart = (DWORD)(aOffset & 0xFFFFFFFF);
			distance.HighPart = (DWORD)(aOffset >> 32);

			{
				BOOL result = SetFilePointerEx(m_internal->m_handle, distance, NULL, FILE_BEGIN);
				JELLY_CHECK(result != 0, "SetFilePointerEx() failed (offset %llu, path %s).", aOffset, m_path.c_str());
			}

			{
				DWORD bytesRead;
				BOOL result = ReadFile(m_internal->m_handle, aBuffer, (DWORD)aBufferSize, &bytesRead, NULL);
				JELLY_CHECK(result != 0, "ReadFile() failed (offset %llu, buffer size %llu, path %s).", aOffset, aBufferSize, m_path.c_str());
				JELLY_ASSERT(bytesRead == (DWORD)aBufferSize);
			}
		#endif
	}

}