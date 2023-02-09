#pragma once

#include "Config.h"
#include "IConfigSource.h"

namespace jelly
{

	/**
	 * \brief Default implementation of the IConfigSource interface. 
	 */
	class DefaultConfigSource
		: public IConfigSource
	{
	public:
					DefaultConfigSource();
		virtual		~DefaultConfigSource();

		/**
		 * Sets the specified configuration item identified by its string name.
		 */
		void		SetString(
						const char*						aString,
						const char*						aValue);

		/**
		 * Sets the specified configuration item.
		 */
		void		Set(
						uint32_t						aConfigId,
						const char*						aValue);

		/**
		 * Clears all configuration items to their defaults.
		 */
		void		Clear();

		//----------------------------------------------------------------------
		// IConfigSource implementation
		uint32_t	GetVersion() override;
		const char*	Get(
						const char*						aId) override;

	private:

		std::unordered_map<std::string, uint32_t>	m_stringIdTable;

		std::optional<std::string>					m_config[Config::NUM_IDS];

		uint32_t									m_version;
	};

}