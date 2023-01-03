#pragma once

#include <map>
#include <mutex>

#include "CompactionResult.h"
#include "IHost.h"
#include "IStoreWriter.h"
#include "NodeConfig.h"
#include "Result.h"
#include "Queue.h"
#include "WAL.h"

namespace jelly
{

	// Base class for LockNode and BlobNode. Contains a bunch of shared functionality as they are 
	// conceptually very similar. 
	template 
	<
		typename _KeyType,
		typename _RequestType,
		typename _ItemType,
		typename _STLKeyHasher
	>
	class Node
	{
	public:		
		Node(
			IHost*				aHost,
			uint32_t			aNodeId,
			const NodeConfig&	aConfig)
			: m_requestsWriteIndex(0)
			, m_nextWALId(0)
			, m_nextStoreId(0)
			, m_host(aHost)
			, m_nodeId(aNodeId)
			, m_config(aConfig)
			, m_stopped(false)
		{
			for(uint32_t i = 0; i < m_config.m_walConcurrency; i++)
				m_pendingWALs.push_back(NULL);
		}

		~Node()
		{
			for(WAL* wal : m_wals)
				delete wal;
		}

		// Stop accepting new requests and cancel all pending requests, including the ones currently waiting for a WAL.
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
					for (_RequestType* request = m_requests[i].m_first; request != NULL; request = request->m_next)
						pendingRequests.push_back(request);

					m_requests[i].Reset();
				}
			}

			// Cancel all pending requests
			for(_RequestType* request : pendingRequests)
			{
				request->m_result = RESULT_CANCELED;
				request->m_completed.Signal();
			}

			// Notify WALs
			for(WAL* wal : m_wals)
			{
				wal->Cancel();
			}
		}

		// Process all pending requests and return the number of requests processed. Must be called on the main thread.
		uint32_t
		ProcessRequests()
		{
			Queue<_RequestType>* queue = NULL;

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
				for (_RequestType* request = queue->m_first; request != NULL; request = request->m_next)
				{
					JELLY_ASSERT(request->m_result == RESULT_NONE);
					JELLY_ASSERT(!request->m_completed.Poll());
					JELLY_ASSERT(request->m_callback);

					request->m_callback();
				}

				_RequestType* request = queue->m_first;
				while (request != NULL)
				{
					// We need to store 'next' before signaling completion as the waiting thread might delete the request immediately
					_RequestType* next = request->m_next;

					if (!request->m_hasPendingWrite)
						request->m_completed.Signal();

					request = next;
				}
			}

			queue->Reset();

			return count;
		}

		// Flush the specified concurrent WAL. This must be done after ProcessRequests(), but doesn't need to be
		// on the main thread. Requests that required stuff to be written to a WAL isn't going to be flagged as
		// completed before the WAL has been flushed.
		void
		FlushPendingWAL(
			uint32_t		aWALConcurrentIndex)
		{
			JELLY_ASSERT(aWALConcurrentIndex < m_pendingWALs.size());

			WAL* pendingWAL = m_pendingWALs[aWALConcurrentIndex];
			if(pendingWAL != NULL)
				pendingWAL->GetWriter()->Flush();
		}

		// Flush the pending store to permanent storage. Must be called from the main thread.
		// Items flushed here will remove their reference to their pending WAL.
		void
		FlushPendingStore()
		{			
			if(m_pendingStore.size() == 0)
				return;

			// FIXME: handle failure

			{
				uint32_t storeId = CreateStoreId();

				std::unique_ptr<IStoreWriter> writer(m_host->CreateStore(m_nodeId, storeId));

				JELLY_ASSERT(m_flushPendingStoreCallback);
				m_flushPendingStoreCallback(storeId, writer.get(), &m_pendingStore);
			}

			m_pendingStore.clear();
		}

		// Delete all closed WALs with no references.
		void
		CleanupWALs()
		{
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
				}
			}
		}

		// Perform compaction, i.e. it will find permanent stores that are suitable for merging.
		// This can be called from any thread, but only when compaction operation can be happening
		// at the same time.
		void
		PerformCompaction()
		{
			JELLY_ASSERT(m_compactionCallback);
			JELLY_ASSERT(!m_hasPendingCompaction);
			JELLY_ASSERT(!m_pendingCompactionResult);

			m_hasPendingCompaction = true;
			m_pendingCompactionResult = std::make_unique<CompactionResult<_KeyType, _STLKeyHasher>>();

			std::vector<IHost::StoreInfo> storeInfo;
			m_host->GetStoreInfo(m_nodeId, storeInfo);

			// Never consider the highest (latest) store id for compaction as it could be in the process of writing.
			// Furthermore, obviously, we'll need more than 1 store elligible for compaction - i.e. we need a minimum
			// of 3 stores to proceed.
			if(storeInfo.size() >= 3)
			{
				const IHost::StoreInfo* small1 = NULL;
				const IHost::StoreInfo* small2 = NULL;

				// Find two smallest stores
				for(size_t i = 0; i < storeInfo.size() - 1; i++)
				{
					const IHost::StoreInfo& t = storeInfo[i];

					if(small1 == NULL)
						small1 = &t;
					else if(small2 == NULL)
						small2 = &t;
					else if(small1->m_size > t.m_size)
						small1 = &t;
					else if(small2->m_size > t.m_size)
						small2 = &t;
				}

				JELLY_ASSERT(small1 != NULL && small2 != NULL);

				m_compactionCallback(small1->m_id, small2->m_id, m_pendingCompactionResult.get());
			}
		}

		// When PerformCompaction() has completed (on any thread) this method should be called on the main thread
		// to apply the result of the compaction.
		void
		ApplyCompactionResult()
		{
			JELLY_ASSERT(m_hasPendingCompaction);
			JELLY_ASSERT(m_pendingCompactionResult);

			for(CompactionResult<_KeyType, _STLKeyHasher>::CompactedStore* compactedStore : m_pendingCompactionResult->GetCompactedStores())
			{
				if(compactedStore->m_redirect)
				{	
					JELLY_ASSERT(m_compactionRedirectMap.find(compactedStore->m_storeId) == m_compactionRedirectMap.end());
					
					m_compactionRedirectMap[compactedStore->m_storeId] = compactedStore->m_redirect.release();
				}

				m_host->DeleteStore(m_nodeId, compactedStore->m_storeId);
			}

			m_pendingCompactionResult = NULL;
			m_hasPendingCompaction = false;
		}

		// Data access
		uint32_t			GetNodeId() const { return m_nodeId; }
		bool				IsStopped() const { std::lock_guard lock(m_requestsLock); return m_stopped; }

	protected:

		typedef std::unordered_map<_KeyType, _ItemType*, _STLKeyHasher> TableType;
		typedef std::map<_KeyType, _ItemType*> PendingStoreType;
		typedef std::function<void(uint32_t, uint32_t, CompactionResult<_KeyType, _STLKeyHasher>*)> CompactionCallback;
		typedef std::function<void(uint32_t, uint32_t, CompactionResult<_KeyType, _STLKeyHasher>*)> CompactionCallback;
		typedef std::function<void(uint32_t, IStoreWriter*, PendingStoreType*)> FlushPendingStoreCallback;
		typedef CompactionRedirect<_KeyType, _STLKeyHasher> CompactionRedirectType;
		typedef std::unordered_map<uint32_t, CompactionRedirectType*> CompactionRedirectMap;

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
			TableType::iterator i = m_table.find(aKey);
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
			m_table[aKey] = aValue;
		}

		void
		AddRequestToQueue(
			_RequestType*			aRequest)
		{
			aRequest->m_timeStamp = m_host->GetTimeStamp();

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
				aRequest->m_result = RESULT_CANCELED;
				aRequest->m_completed.Signal();
			}
		}

		void
		WriteToWAL(
			_ItemType*				aItem,
			CompletionEvent*		aCompletionEvent,
			Result*					aResult)
		{
			if (aItem->m_pendingWAL != NULL)
			{
				aItem->m_pendingWAL->RemoveReference();
				aItem->m_pendingWAL = NULL;
			}
			else
			{
				m_pendingStore.insert(std::pair<const _KeyType, _ItemType*>(aItem->m_key, aItem));
			}

			{
				uint32_t walConcurrencyIndex = 0;

				{
					_STLKeyHasher hasher;
					size_t hash = hasher(aItem->m_key);
					walConcurrencyIndex = (uint32_t)((size_t)m_config.m_walConcurrency * (hash >> 32) / 0x100000000);
				}

				WAL* wal = _GetPendingWAL(walConcurrencyIndex);

				wal->GetWriter()->WriteItem(aItem, aCompletionEvent, aResult);
								
				aItem->m_pendingWAL = wal;
				aItem->m_pendingWAL->AddReference();
			}		
		}

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

		void
		SetNextStoreId(
			uint32_t		aNextStoreId)
		{
			std::lock_guard lock(m_nextStoreIdLock);
			m_nextStoreId = aNextStoreId;
		}

		IHost*														m_host;
		uint32_t													m_nodeId;
		NodeConfig													m_config;
		TableType													m_table;
		uint32_t													m_nextWALId;		
		CompactionCallback											m_compactionCallback;
		FlushPendingStoreCallback									m_flushPendingStoreCallback;
		PendingStoreType											m_pendingStore;
		CompactionRedirectMap										m_compactionRedirectMap;
		bool														m_stopped;

	private:

		std::mutex													m_nextStoreIdLock;
		uint32_t													m_nextStoreId;

		std::mutex													m_requestsLock;
		uint32_t													m_requestsWriteIndex;
		Queue<_RequestType>											m_requests[2];

		std::vector<WAL*>											m_pendingWALs;
		std::vector<WAL*>											m_wals;

		std::atomic_bool											m_hasPendingCompaction;
		std::unique_ptr<CompactionResult<_KeyType, _STLKeyHasher>>	m_pendingCompactionResult;

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

				pendingWAL = AddWAL(id, m_host->CreateWAL(m_nodeId, id));
			}

			return pendingWAL;
		}
	};

}