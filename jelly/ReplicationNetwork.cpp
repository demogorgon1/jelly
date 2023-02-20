#include <jelly/Base.h>

#include <jelly/ErrorUtils.h>
#include <jelly/ReplicationNetwork.h>

namespace jelly
{

	ReplicationNetwork::ReplicationNetwork(
		uint32_t						aLocalNodeId)
		: m_localNodeId(aLocalNodeId)
		, m_localModeIsMaster(false)
		, m_sendQueue(NULL)
	{

	}

	ReplicationNetwork::~ReplicationNetwork()
	{

	}

	void			
	ReplicationNetwork::SetSendCallback(
		SendCallback					aSendCallback)
	{
		m_sendCallback = aSendCallback;
	}

	void
	ReplicationNetwork::SetNodeIds(
		const std::vector<uint32_t>&	aNodeIds)
	{
		JELLY_ASSERT(aNodeIds.size() > 0);
		m_nodeIds = aNodeIds;
		m_localModeIsMaster = (aNodeIds[0] == m_localNodeId);
	}

	void
	ReplicationNetwork::Send(
		Stream::Writer&					aStream)
	{
		JELLY_ASSERT(m_localModeIsMaster);

		std::lock_guard lock(m_sendQueueLock);
		m_sendQueue.Append(aStream);
	}

	void
	ReplicationNetwork::Update()
	{
		JELLY_ASSERT(m_sendCallback);

		std::unique_ptr<Stream::Buffer> head;

		{
			std::lock_guard lock(m_sendQueueLock);
			head.reset(m_sendQueue.DetachBuffers());
		}

		if(m_localModeIsMaster && head)
		{
			for (size_t i = 1; i < m_nodeIds.size(); i++)
				m_sendCallback(m_nodeIds[i], head.get());
		}
	}

}