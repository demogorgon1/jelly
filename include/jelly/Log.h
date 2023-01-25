#pragma once

/**
 * \file Log.h
 * 
 * Global logging functionality.
 */

namespace jelly
{

	namespace Log
	{

		//! Logging levels
		enum Level
		{
			LEVEL_INFO,			
			LEVEL_WARNING,
			LEVEL_ERROR
		};

		//! Global callback for logging events.
		typedef std::function<void(Level, const char*)> Callback;

		//! Converts a logging level to a string identifier.
		const char*	LevelToString(
						Level			aLevel);

		//! Set global callback for logging events. Not thread safe.
		void		SetCallback(
						Callback		aCallback);

		//! Submits a log message.
		void		Print(
						Level			aLevel,
						const char*		aMessage);

		//! Submits a formatted string as a log message.
		void		PrintF(
						Level			aLevel,
						const char*		aFormat,
						...);

	}

}