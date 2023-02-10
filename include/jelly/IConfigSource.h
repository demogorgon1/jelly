#pragma once

namespace jelly
{

	/**
	 * \brief Abstract interface to configuration provided by the application.
	 * \see DefaultConfigSource
	 */
	class IConfigSource
	{
	public:
		virtual				~IConfigSource() {}

		//----------------------------------------------------------------------------
		// Virtual interface

		/**
		 * \brief The version should be incremented every time anything changes in the
		 * configuration. This will cause any ConfigProxy objects to invalidate their
		 * data.
		 */
		virtual uint32_t	GetVersion() const = 0;

		/**
		 * \brief This should return the string value of the specified configuration
		 * item.
		 */
		virtual const char*	Get(
								const char*		aId) const = 0;	
	};

}