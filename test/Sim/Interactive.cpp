#include <jelly/API.h>

#include "../Config.h"

#include "GameServer.h"
#include "Interactive.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	Interactive::Interactive(
		Network*		aNetwork)
		: m_network(aNetwork)
	{
		// This is really stupid... is there a portable C++ way to have unblocking console input?
		m_inputThread = std::make_unique<std::thread>(_ThreadMain, this);
	}

	Interactive::~Interactive()
	{
		m_inputThread->join();
		m_inputThread.reset();
	}

	bool
	Interactive::Update()
	{
		std::vector<std::string> input;

		{
			std::lock_guard lock(m_inputLock);
			input = std::move(m_input);
		}		

		if(input.size() > 0)
		{
			if(input.size() == 1 && input[0] == "q")
			{
				return false;
			}
			else if(input.size() == 2 && input[0] == "cladd")
			{
				uint32_t count = (uint32_t)atoi(input[1].c_str());

				m_network->m_clientLimit = std::min<uint32_t>((uint32_t)m_network->m_clientLimit + count, m_network->m_config->m_simNumClients);

				printf("Client limit: %u\n", (uint32_t)m_network->m_clientLimit);
			}
			else if (input.size() == 2 && input[0] == "clsub")
			{
				uint32_t count = (uint32_t)atoi(input[1].c_str());

				if(count > m_network->m_clientLimit)
					m_network->m_clientLimit = 0;
				else
					m_network->m_clientLimit -= count;

				printf("Client limit: %u\n", (uint32_t)m_network->m_clientLimit);
			}
			else if(input.size() == 2 && input[0] == "disc")
			{
				size_t count = (size_t)atoi(input[1].c_str());

				if(count > 0 && m_network->m_gameServers.size() > 0)
				{
					size_t disconnectsPerGameServer = count / m_network->m_gameServers.size();
					
					for(size_t i = 0; i < m_network->m_gameServers.size() - 1; i++)
					{
						JELLY_ALWAYS_ASSERT(disconnectsPerGameServer <= count);
						count -= disconnectsPerGameServer;
						m_network->m_gameServers[i]->TestUngracefulDisconnectRandomClients(disconnectsPerGameServer);
					}

					if(count > 0)
					{
						m_network->m_gameServers[m_network->m_gameServers.size() - 1]->TestUngracefulDisconnectRandomClients(count);
					}
				}				
			}
			else
			{
				printf("Invalid input.\n");
			}
		}

		return true;
	}	

	//---------------------------------------------------------------------------

	void		
	Interactive::_ThreadMain(
		Interactive*	aInteractive)
	{
		for(;;)
		{
			char buffer[256];
			if(fgets(buffer, sizeof(buffer) - 1, stdin) != NULL)
			{	
				for(size_t i = 0; i < strlen(buffer); i++)
				{
					if(buffer[strlen(buffer) - i - 1] == '\n')
						buffer[strlen(buffer) - i - 1] = '\0';
					else
						break;
				}			

				std::stringstream tokenizer(buffer);
				std::string token;
				std::vector<std::string> tokens;
				while (std::getline(tokenizer, token, ' '))
					tokens.push_back(token);

				if(tokens.size() > 0 && tokens[0] == "q")
					break;

				std::lock_guard lock(aInteractive->m_inputLock);
				aInteractive->m_input = std::move(tokens);
			}			
		}
	}

}