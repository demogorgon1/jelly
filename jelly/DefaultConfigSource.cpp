#include <jelly/Base.h>

#include <jelly/DefaultConfigSource.h>

namespace jelly
{

	DefaultConfigSource::DefaultConfigSource()
		: m_version(0)
	{
		for (uint32_t i = 0; i < (uint32_t)Config::NUM_IDS; i++)
		{
			const jelly::Config::Info* info = jelly::Config::GetInfo(i);

			m_stringIdTable[info->m_id] = i;
		}

		JELLY_ASSERT(m_stringIdTable.size() == (size_t)Config::NUM_IDS);
	}
	
	DefaultConfigSource::~DefaultConfigSource()
	{

	}

	bool		
	DefaultConfigSource::SetString(
		const char*						aString,
		const char*						aValue) noexcept
	{
		JELLY_ASSERT(aValue != NULL);

		std::unordered_map<std::string, uint32_t>::const_iterator i = m_stringIdTable.find(aString);
		if(i == m_stringIdTable.end())
		{
			Log::PrintF(Log::LEVEL_WARNING, "DefaultConfigSource: invalid configuration id: %s (attempted to set it to '%s')", aString, aValue);
			return false;
		}

		m_config[i->second] = aValue;

		m_version++;

		return true;
	}

	void
	DefaultConfigSource::Set(
		uint32_t						aConfigId,
		const char*						aValue) noexcept
	{
		JELLY_ASSERT(aConfigId < Config::NUM_IDS);
		JELLY_ASSERT(aValue != NULL);

		m_config[aConfigId] = aValue;

		m_version++;
	}

	void
	DefaultConfigSource::Clear() noexcept
	{
		for (uint32_t i = 0; i < (uint32_t)Config::NUM_IDS; i++)
			m_config[i].reset();

		m_version++;
	}

	//--------------------------------------------------------------------------

	uint32_t	
	DefaultConfigSource::GetVersion() const noexcept
	{
		return m_version;
	}

	const char* 
	DefaultConfigSource::Get(
		const char*						aId) const noexcept
	{
		std::unordered_map<std::string, uint32_t>::const_iterator i = m_stringIdTable.find(aId);
		JELLY_ASSERT(i != m_stringIdTable.end());

		uint32_t configId = i->second;
		const std::optional<std::string>& config = m_config[configId];

		if(config.has_value())
			return config.value().c_str();

		return Config::GetInfo(configId)->m_default;
	}

}