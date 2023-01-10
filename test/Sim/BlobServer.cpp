//#include "BlobServer.h"
//#include "Network.h"
//
//namespace jelly::Test::Sim
//{
//
//	BlobServer::BlobServer(
//		Network*	aNetwork,
//		uint32_t	aId)
//		: m_network(aNetwork)
//		, m_id(aId)
//		, m_state(STATE_INIT)
//		, m_accumRequestCount(0)
//		, m_hasNode(false)
//	{
//
//	}
//
//	BlobServer::~BlobServer()
//	{
//
//	}
//
//	void
//	BlobServer::Update()
//	{
//		switch(m_state)
//		{
//		case STATE_INIT:
//			{
//				BlobNodeType::Config config;
//
//				m_node = std::make_unique<BlobNodeType>(&m_network->m_host, m_id, config);
//				m_hasNode = true;
//
//				m_flushPendingWALsTimer.SetTimeout(1000);
//				m_performCompactionTimer.SetTimeout(60 * 1000);
//
//				m_state = STATE_RUNNING;
//			}
//			break;
//
//		case STATE_RUNNING:
//			{
//				uint32_t numProcessedRequests = m_node->ProcessRequests();
//				
//				m_accumRequestCount += numProcessedRequests;
//
//				if (m_accumRequestCount > 0 && m_flushPendingWALsTimer.HasExpired())
//				{
//					m_node->FlushPendingWAL(0);
//
//					m_accumRequestCount = 0;
//				}
//
//				//if(m_performCompactionTimer.HasExpired())
//				//{
//				//	m_node->PerformCompaction();
//				//}
//			}
//			break;
//		}
//	}
//
//}
