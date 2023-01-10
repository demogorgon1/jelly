//#pragma once
//
//#include <jelly/LockNode.h>
//#include <jelly/UIntKey.h>
//#include <jelly/UIntLock.h>
//
//#include "Timer.h"
//
//namespace jelly::Test::Sim
//{
//
//	class Network;
//
//	class LockServer
//	{
//	public:
//		typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, UIntKey<uint32_t>::Hasher> LockNodeType;
//
//						LockServer(
//							Network*			aNetwork,
//							uint32_t			aId);
//						~LockServer();
//
//		void			Update();
//
//		// Data access
//		LockNodeType*	GetNode() { return m_hasNode ? m_node.get() : NULL; }
//
//	private:
//
//		Network*						m_network;
//		uint32_t						m_id;
//
//		std::atomic_bool				m_hasNode;
//		std::unique_ptr<LockNodeType>	m_node;
//
//		uint32_t						m_accumRequestCount;
//		Timer							m_flushPendingWALsTimer;
//
//		enum State
//		{
//			STATE_INIT,
//			STATE_RUNNING
//		};
//
//		State							m_state;
//	};
//
//}

#pragma once

#include <jelly/LockNode.h>
#include <jelly/UIntKey.h>
#include <jelly/UIntLock.h>

#include "NodeServer.h"

namespace jelly::Test::Sim
{

	namespace LockServer
	{

		typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, UIntKey<uint32_t>::Hasher> LockNodeType;

		typedef NodeServer<LockNodeType, NODE_SERVER_TYPE_BLOB> LockServerType;

	}

}
