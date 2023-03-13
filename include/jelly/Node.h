#pragma once

#include "Backup.h"
#include "CompactionJob.h"
#include "CompactionResult.h"
#include "ConfigProxy.h"
#include "FileStatsContext.h"
#include "IHost.h"
#include "IStoreWriter.h"
#include "ItemHashTable.h"
#include "PerfTimer.h"
#include "Queue.h"
#include "ReplicationNetwork.h"
#include "RequestResult.h"
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
		bool _CompressWAL
	>
	class Node
	{
	public:		
		typedef CompactionResult<_KeyType> CompactionResultType;
		typedef Backup<_KeyType, _ItemType> BackupType;

		Node(
			IHost*				aHost,
			uint32_t			aNodeId)
			: m_statsContext(aHost->GetStats())
			, m_requestsWriteIndex(0)
			, m_nextWALId(0)
			, m_nextStoreId(0)
			, m_host(aHost)
			, m_nodeId(aNodeId)
			, m_stopped(false)
			, m_pendingStoreWALItemCount(0)
			, m_currentCompactionIsMajor(false)
			, m_config(aHost->GetConfigSource())
			, m_replicationNetwork(NULL)
		{
			{
				// Make sure that no other node operates at the same root, with the same node id
				m_fileLock.reset(aHost->CreateNodeLock(aNodeId));
			}

			m_pendingWALs.resize((size_t)m_config.GetUInt32(Config::ID_WAL_CONCURRENCY), NULL);
			m_pendingWALsLowPrio.resize((size_t)m_config.GetUInt32(Config::ID_WAL_CONCURRENCY_LOW_PRIO), NULL);

			m_lowPrioRequestReplicationEnabled = m_config.GetBool(Config::ID_REPLICATE_LOW_PRIO_REQUESTS);
		}

		virtual 
		~Node()
		{			
			Stop();

			for(WAL* wal : m_wals)
				delete wal;
		}

		/**
		 * Initializes a named backup. Must be called from main thread. When processing has finished,
		 * finalize backup with FinalizeBackup().
		 */
		BackupType*
		StartBackup(
			const char*						aName)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_START_BACKUP);
			
			std::string prevName;
			std::string backupPath = m_config.GetString(Config::ID_BACKUP_PATH);
			uint32_t latestStoreId = UINT32_MAX;
			bool incremental = false;

			if(m_config.GetBool(Config::ID_BACKUP_INCREMENTAL))
			{
				if(m_host->GetLatestBackupInfo(m_nodeId, backupPath.c_str(), prevName, latestStoreId))				
					incremental = true;
			}

			std::vector<IHost::StoreInfo> storeInfo;
			m_host->GetStoreInfo(m_nodeId, storeInfo);

			std::vector<uint32_t> includeStoreIds;
			size_t totalIncludedStoreSize = 0;

			for(const IHost::StoreInfo& store : storeInfo)
			{
				if(!incremental || store.m_id > latestStoreId)
				{
					includeStoreIds.push_back(store.m_id);										

					totalIncludedStoreSize += store.m_size;
				}				
			}

			JELLY_CHECK(includeStoreIds.size() > 0, Exception::ERROR_NOTHING_TO_BACKUP, "Incremental=%u", incremental);

			std::unique_ptr<BackupType> backup = std::make_unique<BackupType>(
				m_host,
				m_nodeId, 
				backupPath.c_str(), 
				aName, 
				prevName.c_str(),
				&m_statsContext.m_fileStore);

			if(m_config.GetBool(Config::ID_BACKUP_COMPACTION))
			{
				size_t availableDiskSpace = m_host->GetAvailableDiskSpace();

				JELLY_CHECK(availableDiskSpace >= totalIncludedStoreSize, Exception::ERROR_NOT_ENOUGH_AVAILABLE_SPACE_FOR_BACKUP, "Available=%s;MoreRequired=%s", 
					StringUtils::MakeSizeString(availableDiskSpace).c_str(), 
					StringUtils::MakeSizeString(totalIncludedStoreSize - availableDiskSpace).c_str());

				uint32_t compactionStoreId = CreateStoreId();

				backup->SetCompactionJob(compactionStoreId, storeInfo[0].m_id, includeStoreIds);
				backup->SetIncludeStoreIds({ compactionStoreId });
			}
			else
			{
				backup->SetIncludeStoreIds(includeStoreIds);
			}

			return backup.release();
		}

		/**
		 * When backup processing is finished, finalize it by calling this method. Must
		 * be called from the main thread.
		 */
		void
		FinalizeBackup(
			BackupType*			aBackup)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_FINALIZE_BACKUP);
			JELLY_ASSERT(aBackup != NULL);
		
			if (aBackup->HasCompletedOk())
			{
				CompactionResultType* compactionResult = aBackup->GetCompactionResult();
				if (compactionResult != NULL)
					ApplyCompactionResult(compactionResult);
			}
		}

		/**
		 * Enable replication by setting the replication network.
		 */
		void
		SetReplicationNetwork(
			ReplicationNetwork*	aReplicationNetwork) noexcept
		{
			m_replicationNetwork = aReplicationNetwork;
		}

		/**
		 * Should be called when replication data is received. Returns number of items updated. This must be called 
		 * on the main thread.
		 */
		size_t
		ProcessReplication(
			uint32_t				aSourceNodeId,
			const Stream::Buffer*	aHead)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_PROCESS_REPLICATION);
			JELLY_ASSERT(m_replicationNetwork != NULL);
			
			if (!m_replicationNetwork->IsLocalNodeMaster() && m_replicationNetwork->GetMasterNodeId() == aSourceNodeId)
			{
				Stream::Reader reader(_CompressWAL ? m_host->GetCompressionProvider() : NULL, aHead);

				return m_replicationCallback(&reader);
			}

			return 0;
		}

		/**
		 * Stop accepting new requests and cancel all pending requests, including the ones currently waiting for a WAL.
		 */
		void
		Stop() noexcept
		{
			std::vector<_RequestType*> pendingRequests;

			// Flag request queue as stopped, get any pending requests there are
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
				request->GetCompletion()->OnCancel();

			// Notify WALs
			for(WAL* wal : m_wals)
				wal->Cancel();
		}

		/**
		 * Process all pending requests and return the number of requests processed. Must be called on the main thread.
		 * Returns number of requests processed.
		 */
		size_t
		ProcessRequests()
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_PROCESS_REQUESTS);

			Queue<_RequestType>* queue = NULL;

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idProcessRequestsTime);

			{
				std::lock_guard lock(m_requestsLock);

				JELLY_ASSERT(!m_stopped);

				queue = &m_requests[m_requestsWriteIndex];

				m_requestsWriteIndex = (m_requestsWriteIndex + 1) % 2;
			}

			size_t count = queue->m_count;

			queue->SetGuard();

			if (queue->m_first != NULL)
			{
				if (m_replicationNetwork == NULL || m_replicationNetwork->IsLocalNodeMaster())
				{
					for (_RequestType* request = queue->m_first; request != NULL; request = request->GetNext())
						request->Execute();
				}
				else
				{
					for (_RequestType* request = queue->m_first; request != NULL; request = request->GetNext())
						request->SetResult(REQUEST_RESULT_NOT_MASTER);
				}

				_RequestType* request = queue->m_first;
				while (request != NULL)
				{
					// We need to store 'next' before signaling completion as the waiting thread might delete the request immediately
					_RequestType* next = request->GetNext();

					if (!request->HasPendingWrite() || request->GetResult() == REQUEST_RESULT_EXCEPTION)
						request->GetCompletion()->Signal();

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
		 * completed before the WAL has been flushed. Returns number of items flushed.
		 */
		size_t
		FlushPendingWAL(
			uint32_t		aWALConcurrentIndex = UINT32_MAX)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_FLUSH_PENDING_WAL);

			return _FlushPendingWAL(aWALConcurrentIndex, m_pendingWALs, m_replicationNetwork);
		}

		/**
		 * Flush the specified concurrent low-priority WAL. Like FlushPendingWAL(), this must be done after 
		 * ProcessRequests(), but doesn't need to be on the main thread. Note that low-priority requests are always
		 * signaled for completion immediately after processing. Returns number of items flushed.
		 */
		size_t
		FlushPendingLowPrioWAL(
			uint32_t		aWALConcurrentIndex = UINT32_MAX)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_FLUSH_PENDING_LOW_PRIO_WAL);

			return _FlushPendingWAL(aWALConcurrentIndex, m_pendingWALsLowPrio, m_lowPrioRequestReplicationEnabled ? m_replicationNetwork : NULL);
		}

		/**
		 * Return the number of requests waiting to be flushed to the specified concurrent WAL. This should be
		 * called on the main thread, before any calls to FlushPendingWAL().
		 */
		size_t
		GetPendingWALRequestCount(
			uint32_t		aWALConcurrentIndex) const noexcept
		{
			JELLY_ASSERT((size_t)aWALConcurrentIndex < m_pendingWALs.size());

			const WAL* pendingWAL = m_pendingWALs[aWALConcurrentIndex];
			if (pendingWAL != NULL)
				return pendingWAL->GetWriter()->GetPendingWriteCount();

			return 0;
		}

		/**
		 * Return the number of requests waiting to be flushed to the specified concurrent low-priority WAL. 
		 * This should be called on the main thread, before any calls to FlushPendingWAL().
		 */
		size_t
		GetPendingLowPrioWALRequestCount(
			uint32_t		aWALConcurrentIndex) const noexcept
		{
			JELLY_ASSERT((size_t)aWALConcurrentIndex < m_pendingWALsLowPrio.size());

			const WAL* pendingWAL = m_pendingWALsLowPrio[aWALConcurrentIndex];
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
			JELLY_CONTEXT(Exception::CONTEXT_NODE_FLUSH_PENDING_STORE);

			if (m_pendingStore.size() == 0)
				return 0;

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingStoreTime);

			{
				uint32_t storeId = CreateStoreId();

				std::unique_ptr<IStoreWriter> writer(m_host->CreateStore(m_nodeId, storeId, &m_statsContext.m_fileStore));
				std::vector<size_t> newOffsets;

				JELLY_ASSERT(m_writePendingStoreCallback);
				m_writePendingStoreCallback(writer.get(), newOffsets);

				writer->Flush();

				JELLY_ASSERT(m_finishPendingStoreCallback);
				m_finishPendingStoreCallback(storeId, newOffsets);
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
		GetPendingStoreItemCount() const noexcept
		{
			return m_pendingStore.size();
		}

		/**
		 * Returns the number of items that has been written to WALs for the pending store. This can be larger 
		 * than GetPendingStoreItemCount() if the same item has been written multiple times between pending store 
		 * flushes. Must be called from the main thread.
		 */
		size_t
		GetPendingStoreWALItemCount() const noexcept
		{
			return m_pendingStoreWALItemCount;
		}

		/**
		 * Return total size of all WALs on disk. Must be called from the main thread.
		 */
		size_t
		GetTotalWALSize() const noexcept
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
			JELLY_CONTEXT(Exception::CONTEXT_NODE_CLEANUP_WALS);

			size_t deletedWALs = 0;

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idCleanupWALsTime);

			for (size_t i = 0; i < m_wals.size(); i++)
			{
				WAL* wal = m_wals[i];

				if (wal->GetRefCount() == 0 && wal->IsClosed())
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
			JELLY_CONTEXT(Exception::CONTEXT_NODE_PERFORM_COMPACTION);
			JELLY_ASSERT(aCompactionJob.Validate());

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idPerformCompactionTime);

			std::unique_ptr<CompactionResultType> result(new CompactionResultType());

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				JELLY_CHECK(!m_currentCompactionIsMajor, Exception::ERROR_MAJOR_COMPACTION_IN_PROGRESS);

				for (uint32_t storeId : aCompactionJob.m_storeIds)
					JELLY_CHECK(m_currentCompactionStoreIds.find(storeId) == m_currentCompactionStoreIds.end(), Exception::ERROR_COMPACTION_IN_PROGRESS, "StoreId=%u", storeId);

				for (uint32_t storeId : aCompactionJob.m_storeIds)
					m_currentCompactionStoreIds.insert(storeId);
			}

			Compaction::Perform<_KeyType, _ItemType>(
				m_host,
				m_nodeId,
				&m_statsContext.m_fileStore,
				CreateStoreId(),
				aCompactionJob,
				result.get());

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
			JELLY_CONTEXT(Exception::CONTEXT_NODE_PERFORM_MAJOR_COMPECTION);

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idPerformMajorCompactionTime);

			std::unique_ptr<CompactionResultType> result(new CompactionResultType());

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				JELLY_CHECK(!m_currentCompactionIsMajor && m_currentCompactionStoreIds.size() == 0, Exception::ERROR_COMPACTION_IN_PROGRESS);

				m_currentCompactionIsMajor = true;
			}

			result->SetMajorCompaction(true);

			Compaction::PerformMajorCompaction<_KeyType, _ItemType>(
				m_host,
				m_nodeId,
				&m_statsContext.m_fileStore,
				CreateStoreId(),
				result.get());

			return result.release();
		}

		/**
		 * When PerformCompaction() or PerformMajorCompaction() has completed (on any thread) this method should be 
		 * called on the main thread to apply the result of the compaction.
		 */
		size_t
		ApplyCompactionResult(
			CompactionResultType*						aCompactionResult)
		{
			JELLY_CONTEXT(Exception::CONTEXT_NODE_APPLY_COMPACTION_RESULT);

			ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idApplyCompactionTime);

			size_t updatedItemCount = 0;

			for (const typename CompactionResultType::Item& compactionResultItem : aCompactionResult->GetItems())
			{
				_ItemType* item = m_table.Get(compactionResultItem.m_key);
				JELLY_ASSERT(item != NULL);

				if (compactionResultItem.m_seq == item->GetSeq())
				{
					item->CompactionUpdate(compactionResultItem.m_storeId, compactionResultItem.m_storeOffset);
					updatedItemCount++;
				}
			}

			for (uint32_t storeId : aCompactionResult->GetStoreIds())
				m_host->DeleteStore(m_nodeId, storeId);

			{
				std::lock_guard lock(m_currentCompactionStoreIdsLock);

				if (m_currentCompactionIsMajor)
				{
					JELLY_ASSERT(m_currentCompactionStoreIds.size() == 0);

					m_currentCompactionIsMajor = false;
				}
				else
				{
					for (uint32_t deletedStoreId : aCompactionResult->GetStoreIds())
						m_currentCompactionStoreIds.erase(deletedStoreId);
				}
			}

			return updatedItemCount;
		}

		/**
		 * Creates a new always incrementing store id. Can be called from any thread.
		 */
		uint32_t
		CreateStoreId() noexcept
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
			bool					aLowPrio,
			Completion*				aCompletion)
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
				std::vector<WAL*>& pendingWALs = aLowPrio ? m_pendingWALsLowPrio : m_pendingWALs;

				uint32_t walConcurrencyIndex = 0;

				// Pick a pending WAL to write to, using the hash of item key
				{
					static_assert(sizeof(size_t) >= sizeof(uint64_t));
					size_t hash = (size_t)aItem->GetKey().GetHash();

					walConcurrencyIndex = (uint32_t)(pendingWALs.size() * (hash >> 32) / 0x100000000);
				}

				WAL* wal = _GetPendingWAL(walConcurrencyIndex, pendingWALs);

				// Append to WAL
				wal->GetWriter()->WriteItem(aItem, aCompletion);
								
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

		/**
		 * Scan through all items in the same order they're stored in the hash table. Must happen on main thread. 
		 * Beware that this is very slow and shouldn't be haphazardly. Return false from callback to exit early.
		 */
		void
		ForEach(
			std::function<bool(const _ItemType*)>		aCallback) const
		{
			m_table.ForEach(aCallback);
		}

		//--------------------------------------------------------------------------------------------
		// Data access

		uint32_t			GetNodeId() const noexcept { return m_nodeId; }											///< Returns node id.
		bool				IsStopped() const noexcept { std::lock_guard lock(m_requestsLock); return m_stopped; }	///< Returns whether node has been requested to stop.
		size_t				GetPendingWALCount() const noexcept { return m_pendingWALs.size(); }					///< Returns number of pending WALs.
		size_t				GetPendingLowPrioWALCount() const noexcept { return m_pendingWALsLowPrio.size(); }		///< Returns number of pending low-priority WALs.
		IHost*				GetHost() noexcept { return m_host; }													///< Returns pointer to host object associated with this node.
		FileStatsContext*	GetStoreFileStatsContext() noexcept { return &m_statsContext.m_fileStore; }				///< Returns context for file statistics.

	protected:

		typedef std::map<_KeyType, _ItemType*> PendingStoreType;
		typedef std::function<void(IStoreWriter*, std::vector<size_t>&)> WritePendingStoreCallback;
		typedef std::function<void(uint32_t, const std::vector<size_t>&)> FinishPendingStoreCallback;
		typedef std::function<size_t(Stream::Reader*)> ReplicationCallback;

		struct StatsContext
		{
			StatsContext(
				IStats*						aStats) noexcept
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
			IWALWriter*						aWALWriter) noexcept
		{
			m_wals.push_back(new WAL(aId, aWALWriter));
			return m_wals[m_wals.size() - 1];
		}

		size_t
		GetItemCount() const noexcept
		{
			return m_table.Count();
		}

		void
		AddRequestToQueue(
			_RequestType*			aRequest) noexcept
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
				aRequest->GetCompletion()->OnCancel();
			}
		}

		void
		SetNextStoreId(
			uint32_t		aNextStoreId) noexcept
		{
			std::lock_guard lock(m_nextStoreIdLock);
			m_nextStoreId = aNextStoreId;
		}

		uint32_t
		GetNextStoreId() noexcept
		{
			std::lock_guard lock(m_nextStoreIdLock);
			return m_nextStoreId;
		}

		IHost*														m_host;
		uint32_t													m_nodeId;
		ConfigProxy													m_config;
		ItemHashTable<_KeyType, _ItemType>							m_table;
		uint32_t													m_nextWALId;		
		WritePendingStoreCallback									m_writePendingStoreCallback;
		FinishPendingStoreCallback									m_finishPendingStoreCallback;
		ReplicationCallback											m_replicationCallback;
		PendingStoreType											m_pendingStore;
		bool														m_stopped;
		size_t														m_pendingStoreWALItemCount;
		StatsContext												m_statsContext;
		ReplicationNetwork*											m_replicationNetwork;

	private:

		std::mutex													m_nextStoreIdLock;
		uint32_t													m_nextStoreId;
		
		std::mutex													m_requestsLock;
		uint32_t													m_requestsWriteIndex;
		Queue<_RequestType>											m_requests[2];

		std::vector<WAL*>											m_pendingWALs;
		std::vector<WAL*>											m_pendingWALsLowPrio;
		std::vector<WAL*>											m_wals;

		std::mutex													m_currentCompactionStoreIdsLock;
		std::unordered_set<uint32_t>								m_currentCompactionStoreIds;
		bool														m_currentCompactionIsMajor;

		std::unique_ptr<File>										m_fileLock;
		bool														m_lowPrioRequestReplicationEnabled;

		WAL*
		_GetPendingWAL(
			uint32_t			aConcurrentWALIndex,
			std::vector<WAL*>&	aPendingWALs)
		{
			// Get the pending WAL for the concurrent WAL index - if it's getting too big close it and make a new one

			JELLY_ASSERT((size_t)aConcurrentWALIndex < aPendingWALs.size());

			WAL*& pendingWAL = aPendingWALs[aConcurrentWALIndex];

			if (pendingWAL != NULL)
			{
				if (pendingWAL->GetWriter()->HadFailure() || pendingWAL->GetWriter()->GetSize() > m_config.GetSize(Config::ID_WAL_SIZE_LIMIT))
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

		size_t
		_FlushPendingWAL(
			uint32_t			aConcurrentWALIndex,
			std::vector<WAL*>&	aPendingWALs,
			ReplicationNetwork*	aReplicationNetwork)
		{
			size_t count = 0;

			if (aConcurrentWALIndex == UINT32_MAX)
			{
				// Flush all pending WALs
				for (WAL* pendingWAL : aPendingWALs)
				{
					if (pendingWAL != NULL)
					{
						ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingWALTime);

						count += pendingWAL->GetWriter()->Flush(aReplicationNetwork);
					}
				}
			}
			else
			{
				// Flush only specific pending WAL
				JELLY_ASSERT(aConcurrentWALIndex < m_pendingWALs.size());

				WAL* pendingWAL = aPendingWALs[aConcurrentWALIndex];
				if (pendingWAL != NULL)
				{
					ScopedTimeSampler timeSampler(m_host->GetStats(), m_statsContext.m_idFlushPendingWALTime);

					count += pendingWAL->GetWriter()->Flush(aReplicationNetwork);
				}
			}

			return count;
		}
	};

}