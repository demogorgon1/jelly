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

	void
	DefaultConfigSource::Set(
		uint32_t						aConfigId,
		const char*						aValue)
	{
		JELLY_ASSERT(aConfigId < Config::NUM_IDS);
		m_config[aConfigId] = aValue;

		m_version++;
	}

	void
	DefaultConfigSource::Clear()
	{
		for (uint32_t i = 0; i < (uint32_t)Config::NUM_IDS; i++)
			m_config[i].reset();

		m_version++;
	}

	//--------------------------------------------------------------------------

	uint32_t	
	DefaultConfigSource::GetVersion() 
	{
		return m_version;
	}

	const char* 
	DefaultConfigSource::Get(
		const char*						aId)
	{
		std::unordered_map<std::string, uint32_t>::const_iterator i = m_stringIdTable.find(aId);
		JELLY_CHECK(i != m_stringIdTable.end(), "Invalid configuration id: %s", aId);

		uint32_t configId = i->second;
		std::optional<std::string>& config = m_config[configId];

		if(config.has_value())
			return config.value().c_str();

		return Config::GetInfo(configId)->m_default;
	}

}