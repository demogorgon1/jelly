#pragma once

#include "CompactionAdvisor.h"
#include "CompactionJob.h"
#include "Timer.h"

namespace jelly
{

	class IHost;

	template <typename _NodeType>
	class HousekeepingAdvisor
	{
	public:
		struct Config
		{	
			Config()
				: m_minWALFlushIntervalMS(500)
				, m_cleanupWALIntervalMS(2 * 60 * 1000)
				, m_minCompactionIntervalMS(5 * 1000)
				, m_pendingStoreItemLimit(30000)
				, m_pendingStoreWALItemLimit(300000)
				, m_compactionAdvisorSizeMemory(10)
				, m_compactionAdvisorSizeTrendMemory(10)
				, m_compactionAdvisorStrategy(CompactionAdvisor::STRATEGY_SIZE_TIERED)
				, m_compactionAdvisorStrategyUpdateIntervalMS(10 * 1000)
			{

			}

			uint32_t										m_minWALFlushIntervalMS;
			uint32_t										m_cleanupWALIntervalMS;
			uint32_t										m_minCompactionIntervalMS;
			size_t											m_pendingStoreItemLimit;
			uint32_t										m_pendingStoreWALItemLimit;
			uint32_t										m_compactionAdvisorSizeMemory;
			uint32_t										m_compactionAdvisorSizeTrendMemory;
			CompactionAdvisor::Strategy						m_compactionAdvisorStrategy;
			uint32_t										m_compactionAdvisorStrategyUpdateIntervalMS;
		};
			
		struct Event
		{
			enum Type				
			{
				TYPE_FLUSH_PENDING_WAL,
				TYPE_FLUSH_PENDING_STORE,
				TYPE_CLEANUP_WALS,
				TYPE_PERFORM_COMPACTION
			};

			Event()
				: m_type(Type(0))
				, m_concurrentWALIndex(0)
			{

			}

			Type											m_type;

			CompactionJob									m_compactionJob;		// TYPE_PERFORM_COMPACTION
			uint32_t										m_concurrentWALIndex;	// TYPE_FLUSH_PENDING_WAL
		};

		typedef std::function<void(const Event&)> EventHandler;

		HousekeepingAdvisor(
			IHost*				aHost,
			const _NodeType*	aNode,
			const Config&		aConfig = Config())
			: m_node(aNode)
			, m_config(aConfig)
			, m_compactionAdvisor(
				aNode->GetNodeId(), 
				aHost, 
				aConfig.m_compactionAdvisorSizeMemory, 
				aConfig.m_compactionAdvisorSizeTrendMemory,
				aConfig.m_compactionAdvisorStrategyUpdateIntervalMS,
				aConfig.m_compactionAdvisorStrategy)
		{
			// Initialize concurrent WAL state
			{
				m_concurrentWALState.resize(m_node->GetPendingWALCount());

				for(ConcurrentWALState& concurrentWALState : m_concurrentWALState)
					concurrentWALState.m_flushCooldown.SetTimeout(m_config.m_minWALFlushIntervalMS);
			}

			// Initialize WAL cleanup timer
			{
				m_cleanupWALsCooldown.SetTimeout(m_config.m_cleanupWALIntervalMS);
			}

			// Initialize compaction timer
			{
				m_compactionUpdateTimer.SetTimeout(m_config.m_minCompactionIntervalMS);
			}
		}

		~HousekeepingAdvisor()
		{

		}

		void	
		Update(
			EventHandler		aEventHandler)
		{
			_UpdateConcurrentWALState(aEventHandler);
			_UpdatePendingStoreState(aEventHandler);
			_UpdateCleanupWALs(aEventHandler);
			_UpdateCompaction(aEventHandler);
		}

	private:

		static Event EventFlushPendingWAL(uint32_t aConcurrentWALIndex) { Event t; t.m_type = Event::TYPE_FLUSH_PENDING_WAL; t.m_concurrentWALIndex = aConcurrentWALIndex; return t; }
		static Event EventFlushPendingStore() { Event t; t.m_type = Event::TYPE_FLUSH_PENDING_STORE; return t; }
		static Event EventCleanupWALs() { Event t; t.m_type = Event::TYPE_CLEANUP_WALS; return t; }
		static Event EventPerformCompaction(const CompactionJob& aCompactionJob) { Event t; t.m_type = Event::TYPE_PERFORM_COMPACTION; t.m_compactionJob = aCompactionJob; return t; }

		const _NodeType*									m_node;
		Config												m_config;

		struct ConcurrentWALState
		{	
			Timer											m_flushCooldown;
		};

		std::vector<ConcurrentWALState>						m_concurrentWALState;
		Timer												m_cleanupWALsCooldown;
		Timer												m_compactionUpdateTimer;
		CompactionAdvisor									m_compactionAdvisor;

		void
		_UpdateConcurrentWALState(
			EventHandler		aEventHandler)
		{
			for (size_t i = 0; i < m_concurrentWALState.size(); i++)
			{
				ConcurrentWALState& concurrentWALState = m_concurrentWALState[i];

				size_t pendingRequests = m_node->GetPendingWALRequestCount((uint32_t)i);

				if(pendingRequests > 0 && concurrentWALState.m_flushCooldown.HasExpired())
				{
					aEventHandler(EventFlushPendingWAL((uint32_t)i));
				}
			}
		}

		void
		_UpdatePendingStoreState(
			EventHandler		aEventHandler)
		{
			size_t pendingStoreItemCount = m_node->GetPendingStoreItemCount();	
			uint32_t pendingStoreWALItemCount = m_node->GetPendingStoreWALItemCount();

			if(pendingStoreWALItemCount > m_config.m_pendingStoreWALItemLimit || pendingStoreItemCount > m_config.m_pendingStoreItemLimit)
				aEventHandler(EventFlushPendingStore());
		}

		void
		_UpdateCleanupWALs(
			EventHandler		aEventHandler)
		{
			if(m_cleanupWALsCooldown.HasExpired())
				aEventHandler(EventCleanupWALs());
		}
		
		void
		_UpdateCompaction(
			EventHandler		aEventHandler)
		{
			if(m_compactionUpdateTimer.HasExpired())
			{
				m_compactionAdvisor.Update();

				CompactionJob suggestion;
				while((suggestion = m_compactionAdvisor.GetNextSuggestion()).IsSet())
					aEventHandler(EventPerformCompaction(suggestion));
			}
		}

	};

}