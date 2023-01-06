#pragma once

#include <jelly/LockNode.h>
#include <jelly/UIntKey.h>
#include <jelly/UIntLock.h>

namespace jelly::Test::Sim
{

	class Network;

	class LockServer
	{
	public:
		typedef LockNode<UIntKey<uint32_t>, UIntLock<uint32_t>, UIntKey<uint32_t>::Hasher> LockNodeType;

				LockServer(
					Network*			aNetwork,
					uint32_t			aId);
				~LockServer();

		void	Update();

	private:

		Network*						m_network;
		uint32_t						m_id;

		std::unique_ptr<LockNodeType>	m_node;

		enum State
		{
			STATE_INIT,
			STATE_RUNNING
		};

		State							m_state;
	};

}