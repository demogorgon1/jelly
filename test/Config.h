#pragma once

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
					else if (strcmp(arg, "-writetest") == 0)
					{
						m_writeTest = true;
					}
					else if (strcmp(arg, "-writetestblobcount") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_writeTestBlobCount = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-writetestblobsize") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_writeTestBlobSize = (uint32_t)atoi(aArgs[i + 1]);
						i++;
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
					else if (strcmp(arg, "-simtestnostdout") == 0)
					{
						m_simTestStdOut = false;
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
					else if (strcmp(arg, "-simcsvoutput") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						m_simCSVOutput = aArgs[i + 1];
						i++;
					}
					else if (strcmp(arg, "-simcsvoutputcolumns") == 0)
					{
						JELLY_CHECK(i + 1 < aNumArgs, "Syntax error.");
						std::stringstream tokenizer(aArgs[i + 1]);
						std::string token;
						while (std::getline(tokenizer, token, ','))
							m_simCSVOutputColumns.push_back(token);
						i++;
					}
					else
					{
						JELLY_FATAL_ERROR("Syntax error.");
					}
				}
			}

			// NodeTest
			bool						m_hammerTest = false;

			// StepTest
			uint32_t					m_stepTestSeed = 0;
			bool						m_stepTestRandom = false;

			// Sim
			bool						m_simTest = false;
			uint32_t					m_simNumClients = 1;
			uint32_t					m_simClientStartIntervalMS = 10;
			uint32_t					m_simNumGameServers = 1;
			uint32_t					m_simNumLockServers = 1;
			uint32_t					m_simNumBlobServers = 1;
			uint32_t					m_simNumClientThreads = 1;
			uint32_t					m_simNumGameServerThreads = 1;
			uint32_t					m_simNumLockServerThreads = 1;
			uint32_t					m_simNumBlobServerThreads = 1;
			uint32_t					m_simSetIntervalMS = 1000;
			std::string					m_simCSVOutput;
			std::vector<std::string>	m_simCSVOutputColumns;
			bool						m_simTestStdOut = true;

			// WriteTest
			bool						m_writeTest = false;
			uint32_t					m_writeTestBlobCount = 10000;
			uint32_t					m_writeTestBlobSize = 1024;
		};

	}

}