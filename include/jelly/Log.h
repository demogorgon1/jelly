#pragma once

namespace jelly
{

	namespace Log
	{

		enum Level
		{
			LEVEL_INFO,
			LEVEL_WARNING,
			LEVEL_ERROR
		};

		typedef std::function<void(Level, const char*)> Callback;

		const char*	LevelToString(
						Level			aLevel);
		void		SetCallback(
						Callback		aCallback);
		void		Print(
						Level			aLevel,
						const char*		aMessage);
		void		PrintF(
						Level			aLevel,
						const char*		aFormat,
						...);

	}

}