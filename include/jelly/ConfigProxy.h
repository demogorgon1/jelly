#pragma once

#include "Config.h"

namespace jelly
{

	class IHost;

	class ConfigProxy
	{
	public:
								ConfigProxy(
									IHost*			aHost);
								~ConfigProxy();
				
		const char*				GetString(
									Config::Id		aId);
		uint32_t				GetInterval(
									Config::Id		aId);
		uint32_t				GetUInt32(
									Config::Id		aId);
		size_t					GetSize(
									Config::Id		aId);
		bool					GetBool(
									Config::Id		aId);

	private:					

		IHost*		m_host;
	};

}