#pragma once

#include "Sim/BlobServer.h"
#include "Sim/LockServer.h"

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
					else if (strcmp(arg, "-readtest") == 0)
					{
						m_readTest = true;
					}
					else if (strcmp(arg, "-readtestblobcount") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_readTestBlobCount = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-readtestblobcountmemorylimit") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_readTestBlobCountMemoryLimit = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}					
					else if (strcmp(arg, "-writetest") == 0)
					{
						m_writeTest = true;
					}
					else if (strcmp(arg, "-writetestblobcount") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_writeTestBlobCount = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-writetestblobsize") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_writeTestBlobSize = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-writetestbuffercompressionlevel") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_writeTestBufferCompressionLevel = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}					
					else if(strcmp(arg, "-steptestseed") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
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
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumClients = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simstartclientlimit") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simStartClientLimit = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simclientstartintervalms") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simClientStartIntervalMS = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}					
					else if (strcmp(arg, "-simnumgameservers") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumGameServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumlockservers") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumLockServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumblobservers") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumBlobServers = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumclientthreads") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumClientThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumgameserverthreads") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumGameServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumlockserverthreads") == 0)
					{
					JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumLockServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumblobserverthreads") == 0)
					{
					JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumBlobServerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simnumsharedworkerthreads") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simNumSharedWorkerThreads = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simsetintervalms") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simSetIntervalMS = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simbuffercompressionlevel") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simBufferCompressionLevel = (uint32_t)atoi(aArgs[i + 1]);
						i++;
					}
					else if (strcmp(arg, "-simcsvoutput") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						m_simCSVOutput = aArgs[i + 1];
						i++;
					}
					else if (strcmp(arg, "-simcsvoutputcolumns") == 0)
					{
						JELLY_ALWAYS_ASSERT(i + 1 < aNumArgs, "Syntax error.");
						std::stringstream tokenizer(aArgs[i + 1]);
						std::string token;
						while (std::getline(tokenizer, token, ','))
							m_simCSVOutputColumns.push_back(token);
						i++;
					}
					else if(strcmp(arg, "-simconfig") == 0)
					{
						std::stringstream tokenizer(aArgs[i + 1]);
						std::string token;
						while (std::getline(tokenizer, token, ','))
						{
							std::stringstream configTokenizer(token.c_str());
							std::string configToken;
							std::vector<std::string> configTokens;
							while (std::getline(configTokenizer, configToken, ':'))
								configTokens.push_back(configToken);

							JELLY_ALWAYS_ASSERT(configTokens.size() == 2, "Syntax error.");

							m_simConfigSource.SetString(configTokens[0].c_str(), configTokens[1].c_str());
						}

						i++;
					}
					else if(strcmp(arg, "-generatedocs") == 0)
					{
						m_generateDocs = true;
					}
					else
					{
						JELLY_ALWAYS_ASSERT(false, "Syntax error.");
					}
				}
			}

			// NodeTest
			bool									m_hammerTest = false;

			// StepTest
			uint32_t								m_stepTestSeed = 0;
			bool									m_stepTestRandom = false;

			// Sim
			bool									m_simTest = false;
			uint32_t								m_simNumClients = 1;
			uint32_t								m_simClientStartIntervalMS = 10;
			uint32_t								m_simNumGameServers = 1;
			uint32_t								m_simNumLockServers = 1;
			uint32_t								m_simNumBlobServers = 1;
			uint32_t								m_simNumClientThreads = 1;
			uint32_t								m_simNumGameServerThreads = 1;
			uint32_t								m_simNumLockServerThreads = 1;
			uint32_t								m_simNumBlobServerThreads = 1;
			uint32_t								m_simNumSharedWorkerThreads = 1;
			uint32_t								m_simSetIntervalMS = 1000;
			uint32_t								m_simStartClientLimit = 0;
			std::string								m_simCSVOutput;
			std::vector<std::string>				m_simCSVOutputColumns;
			std::vector<std::string>				m_simStdOut;
			uint32_t								m_simBufferCompressionLevel = 0;
			DefaultConfigSource						m_simConfigSource;

			// WriteTest
			bool									m_writeTest = false;
			uint32_t								m_writeTestBlobCount = 10000;
			uint32_t								m_writeTestBlobSize = 1024;
			uint32_t								m_writeTestBufferCompressionLevel = 0;

			// ReadTest
			bool									m_readTest = false;
			uint32_t								m_readTestBlobCount = 0;
			uint32_t								m_readTestBlobCountMemoryLimit = UINT32_MAX;

			// Documentation (not a test)
			bool									m_generateDocs = false;
		};

	}

}