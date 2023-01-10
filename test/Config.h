#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <jelly/ErrorUtils.h>

namespace jelly
{

	namespace Test
	{

		struct Config
		{	
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
					else if (strcmp(arg, "-steptestrandom") == 0)
					{
						m_stepTestRandom = true;
					}
					else if(strcmp(arg, "-simtest") == 0)
					{
						m_simTest = true;
					}
					else if (strcmp(arg, "-simnumclients") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumClients = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simclientstartintervalms") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simClientStartIntervalMS = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}					
					else if (strcmp(arg, "-simnumgameservers") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumGameServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumlockservers") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumLockServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumblobservers") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumBlobServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumclientthreads") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumClientThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumgameserverthreads") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumGameServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumlockserverthreads") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumLockServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumblobserverthreads") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simNumBlobServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simsetintervalms") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simSetIntervalMS = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else
					{
						JELLY_FATAL_ERROR("Syntax error.");
					}
				}
			}

			// NodeTest
			bool		m_hammerTest = false;

			// StepTest
			uint32_t	m_stepTestSeed = 0;
			bool		m_stepTestRandom = false;

			// Sim
			bool		m_simTest = false;
			uint32_t	m_simNumClients = 1000;
			uint32_t	m_simClientStartIntervalMS = 10;
			uint32_t	m_simNumGameServers = 1;
			uint32_t	m_simNumLockServers = 1;
			uint32_t	m_simNumBlobServers = 1;
			uint32_t	m_simNumClientThreads = 1;
			uint32_t	m_simNumGameServerThreads = 1;
			uint32_t	m_simNumLockServerThreads = 1;
			uint32_t	m_simNumBlobServerThreads = 1;
			uint32_t	m_simSetIntervalMS = 1000;
		};

	}

}