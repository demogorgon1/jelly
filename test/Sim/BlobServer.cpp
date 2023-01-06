#include "BlobServer.h"
#include "Network.h"

namespace jelly::Test::Sim
{

	BlobServer::BlobServer(
		Network*	aNetwork,
		uint32_t	aId)
		: m_network(aNetwork)
		, m_id(aId)
		, m_state(STATE_INIT)
	{

	}

	BlobServer::~BlobServer()
	{

	}

	void
	BlobServer::Update()
	{
		switch(m_state)
		{
		case STATE_INIT:
			{
				BlobNodeType::Config config;

				m_node = std::make_unique<BlobNodeType>(&m_network->m_host, m_id, config);
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