#pragma once

#include "Compaction.h"
#include "IFileStreamReader.h"
#include "LockNodeItem.h"
#include "LockNodeRequest.h"
#include "Node.h" 
#include "Queue.h"

namespace jelly
{

	/**
	 * \brief A node for locking 
	 * 
	 * \tparam _KeyType			Key type. For example UIntKey.
	 * \tparam _LockType		Lock type. For example UIntLock.
	 * \tparam _LockMetaType	Lock meta data type. For example LockMetaData::StaticSingleBlob.
	 */	
	template 
	<
		typename _KeyType,
		typename _LockType,
		typename _LockMetaType
	>
	class LockNode
		: public Node<
			_KeyType, 
			LockNodeRequest<_KeyType, _LockType, _LockMetaType>, 
			LockNodeItem<_KeyType, _LockType, _LockMetaType>,
			true> // Enable streaming compression of WALs
	{
	public:
		typedef Node<_KeyType, LockNodeRequest<_KeyType, _LockType, _LockMetaType>, LockNodeItem<_KeyType, _LockType, _LockMetaType>, true> NodeBase;

		typedef LockNodeRequest<_KeyType, _LockType, _LockMetaType> Request;
		typedef LockNodeItem<_KeyType, _LockType, _LockMetaType> Item;

		LockNode(
			IHost*												aHost,
			uint32_t											aNodeId)
			: NodeBase(aHost, aNodeId)
		{
			JELLY_CONTEXT(Exception::CONTEXT_LOCK_NODE_INIT);

			_InitStatsContext(&this->m_statsContext);

			_Restore();

			this->m_writePendingStoreCallback = [&](
				IStoreWriter*									aWriter,
				std::vector<size_t>&							/*aOutNewOffsets*/)
			{
				for (std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					aWriter->WriteItem(item);
				}
			};

			this->m_finishPendingStoreCallback = [&](
				uint32_t										/*aStoreId*/,
				const std::vector<size_t>&						/*aNewOffsets*/)
			{
				for (std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;
					typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

					if (runtimeState.m_pendingWAL != NULL)
					{
						runtimeState.m_pendingWAL->RemoveReference();
						runtimeState.m_pendingWAL = NULL;
					}

					if(runtimeState.m_walInstanceCount > 0)
					{
						JELLY_ASSERT(runtimeState.m_walInstanceCount <= this->m_pendingStoreWALItemCount);
						this->m_pendingStoreWALItemCount -= runtimeState.m_walInstanceCount;
						runtimeState.m_walInstanceCount = 0;
					}
				}

				JELLY_ASSERT(this->m_pendingStoreWALItemCount == 0);
			};

			this->m_replicationCallback = [&](
				Stream::Reader*									aReader) -> size_t
			{
				JELLY_ASSERT(this->m_replicationNetwork != NULL);

				size_t count = 0;

				if(!this->m_replicationNetwork->IsLocalNodeMaster())
				{
					while (!aReader->IsEnd())
					{
						Item item;
						if (!item.Read(aReader, NULL))
							break;

						_ReplicateItem(item);

						count++;
					}
				}

				return count;
			};
		}

		virtual
		~LockNode()
		{
		}

		/**
		* Submits a lock request to the queue.
		*
		* Input  | <!-- -->
		* -------|-----------------------------------------------------------------------------------
		* m_key  | Request key.
		* m_lock | Lock id that should be used.
		*
		* Output | <!-- -->
		* -------|-----------------------------------------------------------------------------------
		* m_meta | Latest meta data.
		* m_lock | Previous lock id.
		* m_seq  | Lock sequence number.
		*/
		void
		Lock(
			Request*											aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_LOCK_NODE_LOCK);

				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_LOCK_TIME);

				aRequest->SetResult(_Lock(aRequest));
			});

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits an unlock request to the queue.
		*
		* Input  | <!-- -->
		* -------|-----------------------------------------------------------------------------------
		* m_key  | Request key.
		* m_lock | Lock id that should be used.
		* m_meta | New meta data to save with the lock.
		*/
		void
		Unlock(
			Request*											aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_LOCK_NODE_UNLOCK);

				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_UNLOCK_TIME);
				
				aRequest->SetResult(_Unlock(aRequest));
			});

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits an delete request to the queue.
		*
		* Input | <!-- -->
		* ------|------------------------------------------------------------------------------------
		* m_key | Request key.
		*/
		void
		Delete(
			Request*											aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_LOCK_NODE_DELETE);

				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_LOCK_DELETE_TIME);

				aRequest->SetResult(_Delete(aRequest));
			});

			this->AddRequestToQueue(aRequest);			
		}

	private:
		
		RequestResult
		_Lock(
			Request*											aRequest)
		{
			std::pair<Item*, bool> result = this->m_table.InsertOrUpdate(aRequest->GetKey(), [aRequest]()
			{
				return new Item(aRequest->GetKey(), aRequest->GetLock());
			});

			Item* item = result.first;

			if(!result.second)
			{
				if(!item->GetLock().IsSet())
				{
					// Not locked, just apply lock
					item->SetLock(aRequest->GetLock());
					aRequest->SetLock(_LockType());
				}
				else if(item->GetLock() != aRequest->GetLock())
				{
					// Locked by someone else				
					if(aRequest->IsForced())
					{
						// Force the lock
						item->SetLock(aRequest->GetLock());
						aRequest->SetLock(item->GetLock());
					}
					else
					{
						// Fail
						aRequest->SetLock(item->GetLock());
						return REQUEST_RESULT_ALREADY_LOCKED;
					}
				}
				else
				{
					// Same lock, no need to write anything - return meta data and timestamp					
					aRequest->SetMeta(item->GetMeta());
					aRequest->SetTimeStamp(item->GetTimeStamp());
					aRequest->SetSeq(item->GetSeq());
					return REQUEST_RESULT_OK;
				}
			}

			if(aRequest->GetMeta().has_value())
				std::swap(aRequest->GetMeta().value(), item->GetMeta());
			else
				aRequest->SetMeta(item->GetMeta());

			item->SetTimeStamp(aRequest->GetTimeStamp());
			aRequest->SetSeq(item->IncrementSeq());

			item->RemoveTombstone();

			aRequest->WriteToWAL(this, item);

			return REQUEST_RESULT_OK;
		}

		RequestResult
		_Unlock(
			Request*											aRequest)
		{
			Item* item = this->m_table.Get(aRequest->GetKey());

			if (item == NULL)
			{
				// Doesn't exist
				return REQUEST_RESULT_DOES_NOT_EXIST;
			}
			else if(!item->GetLock().IsSet())
			{
				// It wasn't locked, fail
				return REQUEST_RESULT_NOT_LOCKED;
			}
			else if(item->GetLock() != aRequest->GetLock() && !aRequest->IsForced())
			{
				// Locked by someone else, fail
				return REQUEST_RESULT_ALREADY_LOCKED;
			}
			
			item->GetLock().Clear();

			if(aRequest->GetMeta().has_value())
				item->SetMeta(aRequest->GetMeta().value());

			item->SetTimeStamp(aRequest->GetTimeStamp());
			item->IncrementSeq();
	
			aRequest->WriteToWAL(this, item);

			return REQUEST_RESULT_OK;
		}

		RequestResult
		_Delete(
			Request*											aRequest)
		{
			Item* item = this->m_table.Get(aRequest->GetKey());

			if (item == NULL)
			{
				// Doesn't exist
				return REQUEST_RESULT_DOES_NOT_EXIST;
			}
			else if (item->GetLock().IsSet() && !aRequest->IsForced())
			{
				// It is locked, fail
				return REQUEST_RESULT_ALREADY_LOCKED;
			}

			item->SetMeta(_LockMetaType());
			item->SetTimeStamp(aRequest->GetTimeStamp());
			item->IncrementSeq();

			item->SetTombstoneStoreId(this->GetNextStoreId());

			aRequest->WriteToWAL(this, item);

			return REQUEST_RESULT_OK;
		}

		void
		_ReplicateItem(
			Item&												aItem)
		{
			JELLY_ASSERT(this->m_replicationNetwork != NULL && !this->m_replicationNetwork->IsLocalNodeMaster());

			std::pair<Item*, bool> result = this->m_table.InsertOrUpdate(aItem.GetKey(), [aItem]()
			{
				return new Item(aItem.GetKey(), aItem.GetLock());
			});

			if(result.second || aItem.GetSeq() > result.first->GetSeq())
			{
				result.first->MoveFrom(&aItem);

				this->WriteToWAL(result.first, false, NULL);
			}
		}

		void
		_InitStatsContext(
			typename NodeBase::StatsContext*					aStatsContext)
		{
			aStatsContext->m_fileWAL.m_idRead = Stat::ID_DISK_READ_LOCK_WAL_BYTES;
			aStatsContext->m_fileWAL.m_idWrite = Stat::ID_DISK_WRITE_LOCK_WAL_BYTES;

			aStatsContext->m_fileStore.m_idRead = Stat::ID_DISK_READ_LOCK_STORE_BYTES;
			aStatsContext->m_fileStore.m_idWrite = Stat::ID_DISK_WRITE_LOCK_STORE_BYTES;

			aStatsContext->m_idWALCount = Stat::ID_LOCK_WAL_COUNT;
			aStatsContext->m_idFlushPendingWALTime = Stat::ID_FLUSH_PENDING_LOCK_WAL_TIME;
			aStatsContext->m_idProcessRequestsTime = Stat::ID_PROCESS_LOCK_REQUESTS_TIME;
			aStatsContext->m_idFlushPendingStoreTime = Stat::ID_FLUSH_PENDING_LOCK_STORE_TIME;
			aStatsContext->m_idCleanupWALsTime = Stat::ID_CLEANUP_LOCK_WALS_TIME;
			aStatsContext->m_idPerformCompactionTime = Stat::ID_PERFORM_LOCK_COMPACTION_TIME;
			aStatsContext->m_idPerformMajorCompactionTime = Stat::ID_PERFORM_MAJOR_LOCK_COMPACTION_TIME;
			aStatsContext->m_idApplyCompactionTime = Stat::ID_APPLY_LOCK_COMPACTION_TIME;
		}

		void
		_Restore()
		{
			std::vector<uint32_t> walIds;
			std::vector<uint32_t> storeIds;

			this->m_host->EnumerateFiles(this->m_nodeId, walIds, storeIds);

			for (uint32_t id : storeIds)
			{
				this->SetNextStoreId(id + 1);
				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadStoreStream(this->m_nodeId, id, &this->m_statsContext.m_fileStore));
				if (f)
					_LoadStore(f.get(), id);
			}
			
			for (uint32_t id : walIds)
			{
				WAL* wal = this->AddWAL(id, NULL);

				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadWALStream(this->m_nodeId, id, true, &this->m_statsContext.m_fileWAL));
				if (f)
					_LoadWAL(f.get(), wal);

				if (wal->GetRefCount() > 0)
					this->m_nextWALId = id + 1;
			}

			this->CleanupWALs();
		}

		void
		_LoadWAL(
			IFileStreamReader*			aReader,
			WAL*						aWAL)
		{
			while (!aReader->IsEnd())
			{
				std::unique_ptr<Item> item(new Item());
				if(!item->Read(aReader, NULL))
					break;

				_KeyType key = item->GetKey();

				Item* existing = this->m_table.Get(key);
				if (existing != NULL)
				{
					if (item->GetSeq() > existing->GetSeq())
					{
						typename Item::RuntimeState& existingRuntimeState = existing->GetRuntimeState();

						existing->MoveFrom(item.get());

						if (existingRuntimeState.m_pendingWAL != NULL)
						{
							existingRuntimeState.m_pendingWAL->RemoveReference();
							existingRuntimeState.m_pendingWAL = NULL;
						}
						else
						{
							this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(existing->GetKey(), existing));
						}

						existingRuntimeState.m_pendingWAL = aWAL;
						existingRuntimeState.m_pendingWAL->AddReference();
					}
				}
				else
				{					
					typename Item::RuntimeState& itemRuntimeState = item->GetRuntimeState();

					itemRuntimeState.m_pendingWAL = aWAL;
					itemRuntimeState.m_pendingWAL->AddReference();

					this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(item->GetKey(), item.get()));

					this->m_table.Insert(key, item.release());
				}
			}

			aWAL->SetSize(aReader->GetTotalBytesRead());
		}

		void
		_LoadStore(
			IFileStreamReader*			aReader,
			uint32_t					/*aStoreId*/)
		{
			while (!aReader->IsEnd())
			{
				std::unique_ptr<Item> item(new Item());
				if(!item->Read(aReader, NULL))
					break;

				_KeyType key = item->GetKey();

				Item* existing = this->m_table.Get(key);
				if (existing != NULL)
				{
					if (item->GetSeq() > existing->GetSeq())
						existing->MoveFrom(item.get());
				}
				else
				{
					this->m_table.Insert(key, item.release());
				}
			}
		}

	};

}