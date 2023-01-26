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
	 * \tparam _STLKeyHasher	A way to hash a key. For example UIntKey::Hasher.
	 */	
	template 
	<
		typename _KeyType,
		typename _LockType,
		typename _LockMetaType,
		typename _STLKeyHasher
	>
	class LockNode
		: public Node<
			_KeyType, 
			LockNodeRequest<_KeyType, _LockType, _LockMetaType>, 
			LockNodeItem<_KeyType, _LockType, _LockMetaType>,
			_STLKeyHasher,
			true> // Enable streaming compression of WALs
	{
	public:
		typedef Node<_KeyType, LockNodeRequest<_KeyType, _LockType, _LockMetaType>, LockNodeItem<_KeyType, _LockType, _LockMetaType>, _STLKeyHasher, true> NodeBase;

		struct Config
		{
			NodeConfig		m_node;
		};

		typedef LockNodeRequest<_KeyType, _LockType, _LockMetaType> Request;
		typedef LockNodeItem<_KeyType, _LockType, _LockMetaType> Item;

		LockNode(
			IHost*												aHost,
			uint32_t											aNodeId,
			const Config&										aConfig = Config())
			: NodeBase(aHost, aNodeId, aConfig.m_node)
			, m_lockNodeConfig(aConfig)
		{
			_InitStatsContext(&this->m_statsContext);

			_Restore();

			this->m_flushPendingStoreCallback = [&](
				uint32_t										/*aStoreId*/,
				IStoreWriter*									aWriter,
				NodeBase::PendingStoreType*						/*aPendingStoreType*/)
			{
				for (std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;
					typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

					aWriter->WriteItem(item);

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
		}

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
		*/
		void
		Lock(
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
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
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
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
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				aRequest->SetResult(_Delete(aRequest));
			});

			this->AddRequestToQueue(aRequest);			
		}

	private:
		
		Config			m_lockNodeConfig;

		Result
		_Lock(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->GetKey(), item))
			{
				// Never seen before, apply lock
				this->SetItem(aRequest->GetKey(), item = new Item(aRequest->GetKey(), aRequest->GetLock()));
			}
			else
			{
				if(!item->GetLock().IsSet())
				{
					// Not locked, just apply lock
					item->SetLock(aRequest->GetLock());
					aRequest->SetLock(_LockType());
				}
				else if(item->GetLock() != aRequest->GetLock())
				{
					// Locked by someone else, fail
					aRequest->SetLock(item->GetLock());
					return RESULT_ALREADY_LOCKED;
				}
				else
				{
					// Same lock, no need to write anything - return meta data and timestamp					
					aRequest->SetMeta(item->GetMeta());
					aRequest->SetTimeStamp(item->GetTimeStamp());
					return RESULT_OK;
				}
			}

 			aRequest->SetMeta(item->GetMeta());

			item->SetTimeStamp(aRequest->GetTimeStamp());
			item->IncrementSeq();

			item->RemoveTombstone();

			aRequest->WriteToWAL(this, item);

			return RESULT_OK;
		}

		Result
		_Unlock(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->GetKey(), item))
			{
				// Doesn't exist
				return RESULT_DOES_NOT_EXIST;
			}
			else if(!item->GetLock().IsSet())
			{
				// It wasn't locked, fail
				return RESULT_NOT_LOCKED;
			}
			else if(item->GetLock() != aRequest->GetLock())
			{
				// Locked by someone else, fail
				return RESULT_ALREADY_LOCKED;
			}
			
			item->GetLock().Clear();

			item->SetMeta(aRequest->GetMeta());
			item->SetTimeStamp(aRequest->GetTimeStamp());
			item->IncrementSeq();
	
			aRequest->WriteToWAL(this, item);

			return RESULT_OK;
		}

		Result
		_Delete(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->GetKey(), item))
			{
				// Doesn't exist
				return RESULT_DOES_NOT_EXIST;
			}
			else if (item->GetLock().IsSet())
			{
				// It is locked, fail
				return RESULT_ALREADY_LOCKED;
			}

			item->SetMeta(_LockMetaType());
			item->SetTimeStamp(aRequest->GetTimeStamp());
			item->IncrementSeq();

			item->SetTombstoneStoreId(this->GetNextStoreId());

			aRequest->WriteToWAL(this, item);

			return RESULT_OK;
		}

		void
		_InitStatsContext(
			NodeBase::StatsContext*			aStatsContext)
		{
			aStatsContext->m_fileWAL.m_idRead = Stat::ID_DISK_READ_LOCK_WAL_BYTES;
			aStatsContext->m_fileWAL.m_idWrite = Stat::ID_DISK_WRITE_LOCK_WAL_BYTES;

			aStatsContext->m_fileStore.m_idRead = Stat::ID_DISK_READ_LOCK_STORE_BYTES;
			aStatsContext->m_fileStore.m_idWrite = Stat::ID_DISK_WRITE_LOCK_STORE_BYTES;

			aStatsContext->m_idWALCount = Stat::ID_LOCK_WAL_COUNT;
			aStatsContext->m_idFlushPendingWALTime = Stat::ID_FLUSH_PENDING_LOCK_WAL_TIME;
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

				Item* existing;
				if (this->GetItem(key, existing))
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

					this->SetItem(key, item.release());
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

				Item* existing;
				if (this->GetItem(key, existing))
				{
					if (item->GetSeq() > existing->GetSeq())
						existing->MoveFrom(item.get());
				}
				else
				{
					this->SetItem(key, item.release());
				}
			}
		}

	};

}