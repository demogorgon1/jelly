#pragma once

#include <stdint.h>
#include <string.h>

#include <jelly/ErrorUtils.h>

namespace jelly
{

	namespace Test
	{

		struct Config
		{	
			Config()
				: m_hammerTest(false)
				, m_stepTestSeed(0)
			{

			}

			void
			InitFromCommandLine(
				int		aNumArgs,
				char**	aArgs)
			{
				for(int i = 1; i < aNumArgs; i++)
				{
					const char* arg = aArgs[i];
					
					if(strcmp(arg, "-hammertest") == 0)
					{
						m_hammerTest = true;
					}
					else if(strcmp(arg, "-steptestseed") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_stepTestSeed = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else
					{
						JELLY_FATAL_ERROR("Syntax error.");
					}
				}
			}

			// NodeTest
			bool		m_hammerTest;

			// StepTest
			uint32_t	m_stepTestSeed;
		};

	}

}