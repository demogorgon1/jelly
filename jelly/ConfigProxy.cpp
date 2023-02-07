#include <jelly/Base.h>

#include <jelly/ConfigProxy.h>
#include <jelly/IConfigSource.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	ConfigProxy::ConfigProxy(
		IConfigSource*	aSource)
		: m_source(aSource)
	{

	}
	
	ConfigProxy::~ConfigProxy()
	{

	}

	const char*
	ConfigProxy::GetString(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_STRING);
		const char* p = m_source->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return p;
	}

	uint32_t				
	ConfigProxy::GetInterval(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_INTERVAL);

		CacheItem& cacheItem = m_cache[aId];
		
		if (m_source->GetVersion() != cacheItem.m_version)
		{
			const char* p = m_source->Get(info->m_id);

			cacheItem.m_version = m_source->GetVersion();
			cacheItem.m_u.m_uint32 = StringUtils::ParseInterval(p);
		}

		return cacheItem.m_u.m_uint32;
	}

	uint32_t	
	ConfigProxy::GetUInt32(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_UINT32);

		CacheItem& cacheItem = m_cache[aId];

		if (m_source->GetVersion() != cacheItem.m_version)
		{
			const char* p = m_source->Get(info->m_id);

			cacheItem.m_version = m_source->GetVersion();
			cacheItem.m_u.m_uint32 = StringUtils::ParseUInt32(p);
		}

		return cacheItem.m_u.m_uint32;
	}
	
	size_t		
	ConfigProxy::GetSize(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_SIZE);

		CacheItem& cacheItem = m_cache[aId];

		if (m_source->GetVersion() != cacheItem.m_version)
		{
			const char* p = m_source->Get(info->m_id);

			cacheItem.m_version = m_source->GetVersion();
			cacheItem.m_u.m_size = StringUtils::ParseSize(p);
		}

		return cacheItem.m_u.m_size;
	}
	
	bool		
	ConfigProxy::GetBool(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_BOOL);

		CacheItem& cacheItem = m_cache[aId];

		if (m_source->GetVersion() != cacheItem.m_version)
		{
			const char* p = m_source->Get(info->m_id);

			cacheItem.m_version = m_source->GetVersion();
			cacheItem.m_u.m_bool = StringUtils::ParseBool(p);
		}

		return cacheItem.m_u.m_bool;
	}

}