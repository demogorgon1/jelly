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
			const _NodeType*	aNode)
			: m_node(aNode)
			, m_config(aHost->GetConfigSource())
			, m_cleanupWALsTimer(&m_config, Config::ID_MAX_CLEANUP_WAL_INTERVAL_MS)
			, m_compactionUpdateTimer(&m_config, Config::ID_MIN_COMPACTION_INTERVAL_MS)
		{
			// Initialize compaction advisor
			{
				CompactionAdvisor::Strategy compactionStrategy;
				const char* compactionStrategyName = m_config.GetString(Config::ID_COMPACTION_STRATEGY);				
				if(strcmp(compactionStrategyName, "stcs") == 0)
					compactionStrategy = CompactionAdvisor::STRATEGY_SIZE_TIERED;

				m_compactionAdvisor = std::make_unique<CompactionAdvisor>(
					m_node->GetNodeId(),
					aHost,
					&m_config,
					compactionStrategy);
			}

			// Initialize concurrent WAL state
			{
				m_concurrentWALState.resize(m_node->GetPendingWALCount());

				for(ConcurrentWALState& concurrentWALState : m_concurrentWALState)
					concurrentWALState.m_flushCooldown.SetTimeout(m_config.GetInterval(Config::ID_MIN_WAL_FLUSH_INTERVAL_MS));
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

		ConfigProxy											m_config;

		struct ConcurrentWALState
		{	
			Timer											m_flushCooldown;
		};

		std::vector<ConcurrentWALState>						m_concurrentWALState;
		Timer												m_cleanupWALsTimer;
		Timer												m_compactionUpdateTimer;
		std::unique_ptr<CompactionAdvisor>					m_compactionAdvisor;

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

					concurrentWALState.m_flushCooldown.SetTimeout(m_config.GetInterval(Config::ID_MIN_WAL_FLUSH_INTERVAL_MS));
				}
			}
		}

		void
		_UpdatePendingStoreState(
			EventHandler		aEventHandler)
		{
			size_t pendingStoreItemCount = m_node->GetPendingStoreItemCount();	
			uint32_t pendingStoreWALItemCount = m_node->GetPendingStoreWALItemCount();

			if(pendingStoreWALItemCount > m_config.GetUInt32(Config::ID_PENDING_STORE_WAL_ITEM_LIMIT) 
				|| pendingStoreItemCount > m_config.GetUInt32(Config::ID_PENDING_STORE_ITEM_LIMIT)) 
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
				m_compactionAdvisor->Update();

				CompactionJob suggestion;
				while((suggestion = m_compactionAdvisor->GetNextSuggestion()).IsSet())
					aEventHandler(EventPerformCompaction(suggestion));
			}
		}

	};

}