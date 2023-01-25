#pragma once

namespace jelly
{

	/**
	 * Configuration shared by LockNode and BlobNode
	 */
	struct NodeConfig
	{
		NodeConfig()
			: m_walSizeLimit(64 * 1024 * 1024)
			, m_walConcurrency(1)
		{

		}

		/**
		 * WAL size limit in bytes. When reached the WAL will be closed and a new one started
		 */
		size_t			m_walSizeLimit;		

		/**
		 * Number of WALs to keep open at the same time. Items are assigned to WALs based on keys.
		 * Concurrent WALs can be flushed in a multi-threaded manner.
		 */
		uint32_t		m_walConcurrency;	
	};

}