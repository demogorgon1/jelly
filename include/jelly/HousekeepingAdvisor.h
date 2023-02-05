#pragma once

#include "CompactionAdvisor.h"
#include "CompactionJob.h"
#include "Timer.h"

namespace jelly
{

	class IHost;

	/**
	 * \brief Object for generating housekeeping advice.
	 * 
	 * This housekeeping advisor can help you make decicision about when various \ref Node housekeeping tasks
	 * should be performed. This class will not perform these tasks, it will just tell you when to do them.
	 * 
	 * \code
	 * typedef HousekeepingAdvisor<NodeType> HousekeepingAdvisorType;
	 * HousekeepingAdvisorType housekeepingAdvisor(&host, &node);
	 * 
	 * ...
	 * 
	 * // HousekeepingAdvisor must be updated on the main thread, i.e. the same one you call Node::ProcessRequests() on.
	 * housekeepingAdvisor.Update([](
	 *     const HousekeepingAdvisorType::Event& aEvent)
	 * {
	 *     switch(aEvent.m_type)
	 *     {
	 *     case HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_WAL: 
	 *         // Now is a good time to flush the pending WAL. If using concurrent WALs, aEvent.m_concurrentWALIndex
	 *         // specifies which one. Note that Node::FlushPendingWAL() can be called on any thread, so ideally
	 *         // at this point you should spawn task on a worker thread for it, but for simplicity you can also just
	 *         // call the method directly here.
	 *         break; 
	 * 
	 *     case HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_STORE:
	 *         // Time to flush pending store with Node::FlushPendingStore(). This must be done on the main thread.
	 *         break;
	 * 
	 *     case HousekeepingAdvisorType::Event::TYPE_CLEANUP_WALS:
	 *         // Too many WALs. Time to call Node::CleanupWALs() to free up disk space. Must be done on main thread.
	 *         break;
	 * 
	 *     case HousekeepingAdvisorType::Event::TYPE_PERFORM_COMPACTION:
	 *         // Free up disk space by performing a minor compaction with Node::PerformCompaction(). The event
	 *         // includes a \ref CompactionJob with information associated with the suggested minor compaction, 
	 *         // which should be passed to Node::PerformCompaction(). 
	 *         // This can be done on a worker thread and when it's finished the result should be applied with
	 *         // Node::ApplyCompactionResult() on the main thread.
	 *         break;
	 *     }
	 * });
	 * \endcode
	 * 
	 * \see Node
	 */
	template <typename _NodeType>
	class HousekeepingAdvisor
	{
	public:
		/**
		 * \brief Housekeeping advisor configuration passed to its constructor.
		 */
		struct Config
		{	
			/**
			 * Minimum WAL flushing interval in milliseconds. Minimum time between TYPE_FLUSH_PENDING_WAL events.
			 * If requests are rare the interval might be longer.
			 */
			uint32_t							m_minWALFlushIntervalMS = 500;

			/**
			 * Maximum time betweem TYPE_CLEANUP_WALS events.
			 */
			uint32_t							m_maxCleanupWALIntervalMS = 2 * 60 * 1000;		

			/**
			 * Never suggest minor compactions more often than this. 
			 */

			uint32_t							m_minCompactionIntervalMS = 5 * 1000;

			/**
			 * If number of pending store items exceed this number, a TYPE_FLUSH_PENDING_STORE event will be 
			 * triggered. Note that if one item has multiple writes it still counts as one item.
			 */
			size_t								m_pendingStoreItemLimit = 30000;

			/**
			 * If number of pending items written to WALs exceed this number, a TYPE_FLUSH_PENDING_STORE event
			 * will be triggered. It doesn't matter how many different items we're talking about - it could all
			 * be the same item written repeatedly.
			 */
			uint32_t							m_pendingStoreWALItemLimit = 300000;

			/**
			 * Compaction advisor will monitor total store size on disk over time. This is the max time to look
			 * back, measured in number of calls to Update().
			 */
			uint32_t							m_compactionAdvisorSizeMemory = 10;

			/**
			 * Like m_compactionAdvisorSizeMemory, but for the derivative (change) of total store size.
			 */
			uint32_t							m_compactionAdvisorSizeTrendMemory = 10;

			/**
			 * Compaction strategy to use. 
			 */
			CompactionAdvisor::Strategy			m_compactionAdvisorStrategy = CompactionAdvisor::STRATEGY_SIZE_TIERED;

			/**
			 * How often the compaction strategy should be updated.
			 */
			uint32_t							m_compactionAdvisorStrategyUpdateIntervalMS = 10 * 1000;
		};
			
		/**
		 * \brief Event structure passed to application EventHandler callback during Update().
		 */
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

			Type								m_type;

			CompactionJob						m_compactionJob;		// TYPE_PERFORM_COMPACTION
			uint32_t							m_concurrentWALIndex;	// TYPE_FLUSH_PENDING_WAL
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
				m_cleanupWALsTimer.SetTimeout(m_config.m_maxCleanupWALIntervalMS);
			}

			// Initialize compaction timer
			{
				m_compactionUpdateTimer.SetTimeout(m_config.m_minCompactionIntervalMS);
			}
		}

		~HousekeepingAdvisor()
		{

		}

		//! Updates the housekeeping advisor, invoking the event handler if it wants the application to do something
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
		Timer												m_cleanupWALsTimer;
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
			{
				aEventHandler(EventFlushPendingStore());

				// Always do a cleanup WALs event after flushing pending store
				aEventHandler(EventCleanupWALs());
				m_cleanupWALsTimer.Reset();
			}
		}

		void
		_UpdateCleanupWALs(
			EventHandler		aEventHandler)
		{
			if(m_cleanupWALsTimer.HasExpired())
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