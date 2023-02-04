#pragma once

#include "CompactionJob.h"
#include "CompactionResult.h"
#include "FileStatsContext.h"
#include "IHost.h"
#include "IStoreWriter.h"
#include "NodeConfig.h"
#include "PerfTimer.h"
#include "Queue.h"
#include "Result.h"
#include "ScopedTimeSampler.h"
#include "Stat.h"
#include "WAL.h"

namespace jelly
{

	/**
	 * \brief Base class for LockNode and BlobNode. 
	 *
	 * Contains a bunch of shared functionality as they are conceptually very similar. 
	 * Applications should not use this class template directly.
	 */
	template 
	<
		typename _KeyType,
		typename _RequestType,
		typename _ItemType,
		typename _STLKeyHasher,
		bool _CompressWAL
	>
	class Node
	{
	public:		
		typedef CompactionResult<_KeyType, _STLKeyHasher> CompactionResultType;

		Node(
			IHost*				aHost,
			uint32_t			aNodeId,
			const NodeConfig&	aConfig)
			: m_statsContext(aHost->GetStats())
			, m_requestsWriteIndex(0)
			, m_nextWALId(0)
			, m_nextStoreId(0)
			, m_host(aHost)
			, m_nodeId(aNodeId)
			, m_config(aConfig)
			, m_stopped(false)
			, m_pendingStoreWALItemCount(0)
			, m_currentCompactionIsMajor(false)
		{
			for(uint32_t i = 0; i < m_config.m_walConcurrency; i++)
				m_pendingWALs.push_back(NULL);
		}

		~Node()
		{
			Stop();

			for(WAL* wal : m_wals)
				delete wal;

			for(typename TableType::iterator i = m_table.begin(); i != m_table.end(); i++)
				delete i->second;
		}

		/**
		 * Stop accepting new requests and cancel all pending requests, including the ones currently waiting for a WAL.
		 */
		void
		Stop()
		{
			std::vector<_RequestType*> pendingRequests;

			// Flag request queue as locked, get any pending requests there are
			{
				std::lock_guard lock(m_requestsLock);

				if(m_stopped)
					return;

				m_stopped = true;

				for(size_t i = 0; i < 2; i++)
				{
					for (_RequestType* request = m_requests[i].m_first; request != NULL; request = request->GetNext())
						pendingRequests.push_back(request);

					m_requests[i].Reset();
				}
			}

			// Cancel all pending requests
			for(_RequestType* request : pendingRequests)
			{
				request->SetResult(RESULT_CANCELED);
				request->SignalCompletion();
			}

			// Notify WALs
			for(WAL* wal : m_wals)
				wal->Cancel();
		}

		/**
		 * Process all pending requests and return the number of requests processed. Must be called on the main thread.
		 * Returns number of requests processed.
		 */
		uint32_t
		ProcessRequests()
		{
			Queue<_RequestType>* queue = NULL;

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idProcessRequestsTime);

			{
				std::lock_guard lock(m_requestsLock);

				JELLY_ASSERT(!m_stopped);

				queue = &m_requests[m_requestsWriteIndex];

				m_requestsWriteIndex = (m_requestsWriteIndex + 1) % 2;
			}

			uint32_t count = queue->m_count;

			queue->SetGuard();

			if (queue->m_first != NULL)
			{
				for (_RequestType* request = queue->m_first; request != NULL; request = request->GetNext())
					request->Execute();

				_RequestType* request = queue->m_first;
				while (request != NULL)
				{
					// We need to store 'next' before signaling completion as the waiting thread might delete the request immediately
					_RequestType* next = request->GetNext();

					if (!request->HasPendingWrite())
						request->SignalCompletion();

					request = next;
				}
			}

			queue->Reset();

			m_host->GetStats()->Emit(m_statsContext.m_idWALCount, m_wals.size());

			return count;
		}

		/**
		 * Flush the specified concurrent WAL. This must be done after ProcessRequests(), but doesn't need to be
		 * on the main thread. Requests that required stuff to be written to a WAL isn't going to be flagged as
		 * completed before the WAL has been flushed. 
		 * Returns number of items flushed.
		 */
		size_t
		FlushPendingWAL(
			uint32_t		aWALConcurrentIndex = UINT32_MAX)
		{
			size_t count = 0;

			if(aWALConcurrentIndex == UINT32_MAX)
			{
				// Flush all pending WALs
				for(WAL* pendingWAL : m_pendingWALs)
				{
					if (pendingWAL != NULL)
					{
						ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingWALTime);

						count += pendingWAL->GetWriter()->Flush();
					}
				}
			}
			else
			{
				// Flush only specific pending WAL
				JELLY_ASSERT(aWALConcurrentIndex < m_pendingWALs.size());

				WAL* pendingWAL = m_pendingWALs[aWALConcurrentIndex];
				if (pendingWAL != NULL)
				{
					ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingWALTime);

					count += pendingWAL->GetWriter()->Flush();
				}
			}

			return count;
		}

		/**
		 * Return the number of requests waiting to be flushed to the specified concurrent WAL. This should be
		 * called on the main thread, before any calls to FlushPendingWAL().
		 */
		size_t
		GetPendingWALRequestCount(
			uint32_t		aWALConcurrentIndex) const
		{
			JELLY_ASSERT(aWALConcurrentIndex < m_pendingWALs.size());

			const WAL* pendingWAL = m_pendingWALs[aWALConcurrentIndex];
			if (pendingWAL != NULL)
				return pendingWAL->GetWriter()->GetPendingWriteCount();

			return 0;
		}

		/**
		 * Flush the pending store to permanent storage. Must be called from the main thread.
		 * Items flushed here will remove their reference to their pending WAL.
		 * Returns number of items flushed.
		 */
		size_t
		FlushPendingStore()
		{			
			if(m_pendingStore.size() == 0)
				return 0;

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingStoreTime);

			{
				uint32_t storeId = CreateStoreId();

				std::unique_ptr<IStoreWriter> writer(m_host->CreateStore(m_nodeId, storeId, &m_statsContext.m_fileStore));

				JELLY_ASSERT(m_flushPendingStoreCallback);
				m_flushPendingStoreCallback(storeId, writer.get(), &m_pendingStore);

				writer->Flush();
			}

			size_t count = m_pendingStore.size();
			m_pendingStore.clear();

			return count;
		}

		/**
		 * Return the number of items in the pending store. Each of these items hold a reference to a pending
		 * WAL. Must be called from the main thread.
		 */
		size_t
		GetPendingStoreItemCount() const
		{
			return m_pendingStore.size();
		}

		/**
		 * Returns the number of items that has been written to WALs for the pending store. This can be larger 
		 * than GetPendingStoreItemCount() if the same item has been written multiple times between pending store 
		 * flushes. Must be called from the main thread.
		 */
		uint32_t
		GetPendingStoreWALItemCount() const
		{
			return m_pendingStoreWALItemCount;
		}

		/**
		 * Return total size of all WALs on disk. Must be called from the main thread.
		 */
		size_t
		GetTotalWALSize() const
		{
			size_t totalSize = 0;

			for(WAL* wal : m_wals)
				totalSize += wal->GetSize();

			return totalSize;
		}

		/**
		 * Delete all closed WALs with no references. Must be called from the main thread.
		 */
		size_t
		CleanupWALs()
		{
			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idCleanupWALsTime);

			size_t deletedWALs = 0;

			for(size_t i = 0; i < m_wals.size(); i++)
			{
				WAL* wal = m_wals[i];

				if(wal->GetRefCount() == 0 && wal->IsClosed())
				{
					uint32_t id = wal->GetId();

					delete wal;
					m_wals.erase(m_wals.begin() + i);
					i--;

					m_host->DeleteWAL(m_nodeId, id);

					deletedWALs++;
				}
			}

			return deletedWALs;
		}

		/**
		 * Perform (minor) compaction on the specified store ids. Any tombstones found from before
		 * "min store id" will be evicted. This can be called from any thread. Apply the result of the 
		 * compaction using ApplyCompactionResult() from the main thread.
		 */
		CompactionResultType*
		PerformCompaction(
			const CompactionJob&						aCompactionJob)
		{
			JELLY_ASSERT(aCompactionJob.m_oldestStoreId != UINT32_MAX);
			JELLY_ASSERT(aCompactionJob.m_storeId1 != UINT32_MAX);
			JELLY_ASSERT(aCompactionJob.m_storeId2 != UINT32_MAX);
			JELLY_ASSERT(aCompactionJob.m_storeId1 != aCompactionJob.m_storeId2);

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idPerformCompactionTime);

			std::unique_ptr<CompactionResultType> result(new CompactionResultType());

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				if(m_currentCompactionIsMajor)
					return result.release();

				if (m_currentCompactionStoreIds.find(aCompactionJob.m_storeId1) != m_currentCompactionStoreIds.end())
					return result.release();

				if (m_currentCompactionStoreIds.find(aCompactionJob.m_storeId2) != m_currentCompactionStoreIds.end())
					return result.release();

				m_currentCompactionStoreIds.insert(aCompactionJob.m_storeId1);
				m_currentCompactionStoreIds.insert(aCompactionJob.m_storeId2);
			}

			Compaction::Perform<_KeyType, _ItemType, _STLKeyHasher>(this, aCompactionJob, result.get());

			return result.release();
		}

		/**
		 * Perform major copmaction where all stores (except the latest, which could potentially be work in progress)
		 * will be compacted. This can be called from any thread. Apply the result of the 
		 * compaction using ApplyCompactionResult() from the main thread.
		 */
		CompactionResultType*
		PerformMajorCompaction()
		{
			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idPerformMajorCompactionTime);

			std::unique_ptr<CompactionResultType> result(new CompactionResultType());

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				if(m_currentCompactionIsMajor || m_currentCompactionStoreIds.size() > 0)
					return result.release();

				m_currentCompactionIsMajor = true;
			}

			result->SetMajorCompaction(true);

			Compaction::PerformMajorCompaction<_KeyType, _ItemType, _STLKeyHasher>(this, result.get());

			return result.release();
		}

		/**
		 * When PerformCompaction() or PerformMajorCompaction() has completed (on any thread) this method should be 
		 * called on the main thread to apply the result of the compaction.
		 */
		void
		ApplyCompactionResult(
			CompactionResultType*						aCompactionResult)
		{
			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idApplyCompactionTime);

			std::vector<uint32_t> deletedStoreIds;

			for(typename CompactionResultType::CompactedStore* compactedStore : aCompactionResult->GetCompactedStores())
			{
				if(compactedStore->m_redirect)
				{	
					JELLY_ASSERT(m_currentCompactionIsMajor || m_compactionRedirectMap.find(compactedStore->m_storeId) == m_compactionRedirectMap.end());
					
					m_compactionRedirectMap[compactedStore->m_storeId] = compactedStore->m_redirect.release();
				}

				m_host->DeleteStore(m_nodeId, compactedStore->m_storeId);

				deletedStoreIds.push_back(compactedStore->m_storeId);
			}

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				if(m_currentCompactionIsMajor)
				{
					JELLY_ASSERT(m_currentCompactionStoreIds.size() == 0);

					m_currentCompactionIsMajor = false;
				}
				else
				{
					for (uint32_t deletedStoreId : deletedStoreIds)
						m_currentCompactionStoreIds.erase(deletedStoreId);
				}
			}
		}

		/**
		 * Creates a new always incrementing store id. Can be called from any thread.
		 */
		uint32_t
		CreateStoreId()
		{
			uint32_t id = 0;

			{
				std::lock_guard lock(m_nextStoreIdLock);
				id = m_nextStoreId++;
			}

			return id;
		}

		/**
		 * Write an item to a WAL.
		 */
		void
		WriteToWAL(
			_ItemType*				aItem,
			CompletionEvent*		aCompletionEvent,
			Result*					aResult)
		{
			typename _ItemType::RuntimeState& runtimeState = aItem->GetRuntimeState();

			if (runtimeState.m_pendingWAL != NULL)
			{
				runtimeState.m_pendingWAL->RemoveReference();
				runtimeState.m_pendingWAL = NULL;
			}
			else
			{
				m_pendingStore.insert(std::pair<const _KeyType, _ItemType*>(aItem->GetKey(), aItem));
			}

			{
				uint32_t walConcurrencyIndex = 0;

				// Pick a pending WAL to write to, using the hash of item key
				{
					_STLKeyHasher hasher;
					size_t hash = hasher(aItem->GetKey());
					walConcurrencyIndex = (uint32_t)((size_t)m_config.m_walConcurrency * (hash >> 32) / 0x100000000);
				}

				WAL* wal = _GetPendingWAL(walConcurrencyIndex);

				// Append to WAL
				wal->GetWriter()->WriteItem(aItem, aCompletionEvent, aResult);
								
				runtimeState.m_pendingWAL = wal;
				runtimeState.m_pendingWAL->AddReference();

				// Number of instances that this item exists in different pending WALs (if an item is written repeatedly 
				// (which is likely), many copies of it will exist in the same WALs)
				runtimeState.m_walInstanceCount++;

				// Total number of item instances across all pending WALs. This is a useful metric for deciding when to 
				// flush pending store to disk.
				m_pendingStoreWALItemCount++;
			}		
		}

		//--------------------------------------------------------------------------------------------
		// Data access

		uint32_t			GetNodeId() const { return m_nodeId; }											///< Returns node id.
		bool				IsStopped() const { std::lock_guard lock(m_requestsLock); return m_stopped; }	///< Returns whether node has been requested to stop.
		size_t				GetPendingWALCount() const { return m_pendingWALs.size(); }						///< Returns number of pending WALs.
		IHost*				GetHost() { return m_host; }													///< Returns pointer to host object associated with this node.
		FileStatsContext*	GetStoreFileStatsContext() { return &m_statsContext.m_fileStore; }				///< Returns context for file statistics.

	protected:

		typedef std::unordered_map<_KeyType, _ItemType*, _STLKeyHasher> TableType;
		typedef std::map<_KeyType, _ItemType*> PendingStoreType;
		typedef std::function<void(uint32_t, IStoreWriter*, PendingStoreType*)> FlushPendingStoreCallback;
		typedef CompactionRedirect<_KeyType, _STLKeyHasher> CompactionRedirectType;
		typedef std::unordered_map<uint32_t, CompactionRedirectType*> CompactionRedirectMap;

		struct StatsContext
		{
			StatsContext(
				IStats*						aStats)
				: m_fileStore(aStats)
				, m_fileWAL(aStats)
			{

			}

			FileStatsContext		m_fileStore;
			FileStatsContext		m_fileWAL;

			uint32_t				m_idWALCount = UINT32_MAX;
			uint32_t				m_idFlushPendingWALTime = UINT32_MAX;
			uint32_t				m_idProcessRequestsTime = UINT32_MAX;
			uint32_t				m_idFlushPendingStoreTime = UINT32_MAX;
			uint32_t				m_idCleanupWALsTime = UINT32_MAX;
			uint32_t				m_idPerformCompactionTime = UINT32_MAX;
			uint32_t				m_idPerformMajorCompactionTime = UINT32_MAX;
			uint32_t				m_idApplyCompactionTime = UINT32_MAX;
		};

		WAL*
		AddWAL(
			uint32_t						aId,
			IWALWriter*						aWALWriter)
		{
			m_wals.push_back(new WAL(aId, aWALWriter));
			return m_wals[m_wals.size() - 1];
		}

		bool
		GetItem(
			const _KeyType&					aKey,
			_ItemType*&						aOut)
		{
			typename TableType::iterator i = m_table.find(aKey);
			if (i == m_table.end())
				return false;

			aOut = i->second;
			return true;
		}

		void
		SetItem(
			const _KeyType&					aKey,
			_ItemType*						aValue)
		{
			typename TableType::iterator i = m_table.find(aKey);
			JELLY_ASSERT(i == m_table.end());
			m_table[aKey] = aValue;
		}

		size_t
		GetItemCount() const
		{
			return m_table.size();
		}

		void
		AddRequestToQueue(
			_RequestType*			aRequest)
		{
			aRequest->SetTimeStamp(m_host->GetTimeStamp());

			bool canceled = false;

			{
				std::lock_guard lock(m_requestsLock);

				if(!m_stopped)
					m_requests[m_requestsWriteIndex].Add(aRequest);
				else
					canceled = true;
			}

			if(canceled)
			{
				aRequest->SetResult(RESULT_CANCELED);
				aRequest->SignalCompletion();
			}
		}

		void
		SetNextStoreId(
			uint32_t		aNextStoreId)
		{
			std::lock_guard lock(m_nextStoreIdLock);
			m_nextStoreId = aNextStoreId;
		}

		uint32_t
		GetNextStoreId()
		{
			std::lock_guard lock(m_nextStoreIdLock);
			return m_nextStoreId;
		}

		IHost*														m_host;
		uint32_t													m_nodeId;
		NodeConfig													m_config;
		TableType													m_table;
		uint32_t													m_nextWALId;		
		FlushPendingStoreCallback									m_flushPendingStoreCallback;
		PendingStoreType											m_pendingStore;
		CompactionRedirectMap										m_compactionRedirectMap;
		bool														m_stopped;
		uint32_t													m_pendingStoreWALItemCount;
		StatsContext												m_statsContext;

	private:

		std::mutex													m_nextStoreIdLock;
		uint32_t													m_nextStoreId;
		
		std::mutex													m_requestsLock;
		uint32_t													m_requestsWriteIndex;
		Queue<_RequestType>											m_requests[2];

		std::vector<WAL*>											m_pendingWALs;
		std::vector<WAL*>											m_wals;

		std::mutex													m_currentCompactionStoreIdsLock;
		std::unordered_set<uint32_t>								m_currentCompactionStoreIds;
		bool														m_currentCompactionIsMajor;

		WAL*
		_GetPendingWAL(
			uint32_t		aConcurrentWALIndex)
		{
			// Get the pending WAL for the concurrent WAL index - if it's getting too big close it and make a new one

			JELLY_ASSERT(aConcurrentWALIndex < m_pendingWALs.size());

			WAL*& pendingWAL = m_pendingWALs[aConcurrentWALIndex];

			if (pendingWAL != NULL)
			{
				if (pendingWAL->GetWriter()->GetSize() > m_config.m_walSizeLimit)
				{
					pendingWAL->Close();
					pendingWAL = NULL;
				}
			}

			if (pendingWAL == NULL)
			{
				uint32_t id = m_nextWALId++;

				pendingWAL = AddWAL(id, m_host->CreateWAL(m_nodeId, id, _CompressWAL, &m_statsContext.m_fileWAL));
			}

			return pendingWAL;
		}
	};

}