#pragma once

#include "Config.h"

namespace jelly
{

	class IConfigSource;

	// This acts as a cache of an IConfigSource. Contents get invalidated when
	// IConfigSource::GetVersion() is bumped.
	class ConfigProxy
	{
	public:
								ConfigProxy(
									const IConfigSource*	aSource) noexcept;
								~ConfigProxy();
				
		const char*				GetString(
									Config::Id				aId);
		uint32_t				GetInterval(
									Config::Id				aId);
		uint32_t				GetUInt32(
									Config::Id				aId);
		size_t					GetSize(
									Config::Id				aId);
		bool					GetBool(
									Config::Id				aId);

	private:					

		const IConfigSource*		m_source;

		struct CacheItem
		{
			CacheItem()
				: m_version(UINT32_MAX)
			{

			}

			uint32_t				m_version;
			
			union _CacheItem_u
			{
				size_t				m_size;
				uint32_t			m_uint32;
				bool				m_bool;
			} m_u;
		};

		CacheItem					m_cache[Config::NUM_IDS];
	};

}