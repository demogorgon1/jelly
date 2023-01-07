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
		, m_hasNode(false)
		, m_accumRequestCount(0)
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
				m_hasNode = true;

				m_flushPendingWALsTimer.SetTimeout(500);

				m_state = STATE_RUNNING;
			}
			break;

		case STATE_RUNNING:
			{
				uint32_t numProcessedRequests = m_node->ProcessRequests();
				
				m_accumRequestCount += numProcessedRequests;

				if (m_accumRequestCount > 0 && m_flushPendingWALsTimer.HasExpired())
				{
					m_node->FlushPendingWAL(0);

					m_accumRequestCount = 0;
				}
			}
			break;
		}
	}

}