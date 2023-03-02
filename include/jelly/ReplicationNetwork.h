#pragma once

#include "Stream.h"

namespace jelly
{

	class ReplicationNetwork
	{
	public:
		typedef std::function<void(uint32_t, const Stream::Buffer*)> SendCallback;

						ReplicationNetwork(
							uint32_t						aLocalNodeId) noexcept;
						~ReplicationNetwork();

		void			SetSendCallback(
							SendCallback					aSendCallback) noexcept;
		void			SetNodeIds(							
							const std::vector<uint32_t>&	aNodeIds) noexcept;
		void			Send(
							Stream::Writer&					aStream) noexcept;
		void			Update();

		// Data access
		bool			IsLocalNodeMaster() const noexcept { return m_localModeIsMaster; }
		uint32_t		GetMasterNodeId() const noexcept { JELLY_ASSERT(m_nodeIds.size() > 0); return m_nodeIds[0]; }
	
	private:
		
		uint32_t								m_localNodeId;
		bool									m_localModeIsMaster;
		std::vector<uint32_t>					m_nodeIds;
		SendCallback							m_sendCallback;

		std::mutex								m_sendQueueLock;
		Stream::Writer							m_sendQueue;
	};

}