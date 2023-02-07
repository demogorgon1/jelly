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

		template <typename _T>
		_T 						Get(			
									Config::Id		aId);	

	private:					

		IHost*		m_host;

		uint32_t				_GetUInt32(
									Config::Id		aId);
		size_t					_GetSize(
									Config::Id		aId);
		bool					_GetBool(
									Config::Id		aId);

	};

	template <> inline uint32_t	ConfigProxy::Get<uint32_t>(Config::Id aId)		{ return _GetUInt32(aId); }
	template <> inline size_t	ConfigProxy::Get<size_t>(Config::Id aId)		{ return _GetSize(aId); }
	template <> inline bool		ConfigProxy::Get<bool>(Config::Id aId)			{ return _GetBool(aId); }

}