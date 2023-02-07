#pragma once

#include "Config.h"
#include "IConfigSource.h"

namespace jelly
{

	class DefaultConfigSource
		: public IConfigSource
	{
	public:
					DefaultConfigSource();
		virtual		~DefaultConfigSource();

		void		Set(
						uint32_t						aConfigId,
						const char*						aValue);
		void		Clear();

		// IConfigSource implementation
		const char*	Get(
						const char*						aId);

	private:

		std::unordered_map<std::string, uint32_t>	m_stringIdTable;

		std::optional<std::string>					m_config[Config::NUM_IDS];
	};

}