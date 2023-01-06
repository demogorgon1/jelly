#pragma once

#include <jelly/Blob.h>
#include <jelly/BlobNode.h>
#include <jelly/UIntKey.h>

namespace jelly::Test::Sim
{

	class Network;

	class BlobServer
	{
	public:
		typedef BlobNode<UIntKey<uint32_t>, Blob, UIntKey<uint32_t>::Hasher> BlobNodeType;

				BlobServer(
					Network*		aNetwork,
					uint32_t		aId);
				~BlobServer();

		void	Update();

	private:

		Network*						m_network;
		uint32_t						m_id;

		std::unique_ptr<BlobNodeType>	m_node;

		enum State
		{
			STATE_INIT,
			STATE_RUNNING
		};

		State							m_state;
	};
	
}