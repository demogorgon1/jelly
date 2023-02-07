#pragma once

namespace jelly
{

	class IConfigSource
	{
	public:
		virtual				~IConfigSource() {}

		// Virtual interface
		virtual const char*	Get(
								const char*		aId) = 0;	
	};

}