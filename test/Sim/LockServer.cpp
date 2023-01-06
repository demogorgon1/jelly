#include "LockServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	LockServer::LockServer(
		Network*			aNetwork,
		uint32_t			aId)
		: m_network(aNetwork)
		, m_state(STATE_INIT)
		, m_id(aId)
	{

	}

	LockServer::~LockServer()
	{

	}

	void
	LockServer::Update()
	{
		switch(m_state)
		{
		case STATE_INIT:
			{
				LockNodeType::Config config;

				m_node = std::make_unique<LockNodeType>(&m_network->m_host, m_id, config);
			}
			break;

		case STATE_RUNNING:
			{
				uint32_t numProcessedRequests = m_node->ProcessRequests();
				numProcessedRequests;
			}
			break;
		}
	}

}