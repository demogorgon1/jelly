#include <jelly/Base.h>

#include <jelly/ConfigProxy.h>
#include <jelly/IConfigSource.h>
#include <jelly/IHost.h>
#include <jelly/StringUtils.h>

namespace jelly
{

	ConfigProxy::ConfigProxy(
		IHost*			aHost)
		: m_host(aHost)
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
		const char* p = m_host->GetConfigSource()->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return p;
	}

	uint32_t				
	ConfigProxy::GetInterval(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_INTERVAL);
		const char* p = m_host->GetConfigSource()->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return StringUtils::ParseInterval(p);
	}

	uint32_t	
	ConfigProxy::GetUInt32(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_UINT32);
		const char* p = m_host->GetConfigSource()->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return StringUtils::ParseUInt32(p);
	}
	
	size_t		
	ConfigProxy::GetSize(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_SIZE);
		const char* p = m_host->GetConfigSource()->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return StringUtils::ParseSize(p);
	}
	
	bool		
	ConfigProxy::GetBool(
		Config::Id		aId)
	{
		const Config::Info* info = Config::GetInfo(aId);
		JELLY_ASSERT(info->m_type == Config::TYPE_BOOL);
		const char* p = m_host->GetConfigSource()->Get(info->m_id);
		JELLY_ASSERT(p != NULL);
		return StringUtils::ParseBool(p);
	}

}