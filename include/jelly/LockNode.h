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
	 * A node for storing blobs.
	 * 
	 * @tparam _KeyType			Key type. For example \ref UIntKey.
	 * @tparam _BlobType		Blob type. For example \ref Blob.
	 * @tparam _STLKeyHasher	A way to hash a key. For example \ref UIntKey::Hasher.
	 */	
	template 
	<
		typename _KeyType,
		typename _LockType,
		typename _STLKeyHasher
	>
	class LockNode
		: public Node<
			_KeyType, 
			LockNodeRequest<_KeyType, _LockType>, 
			LockNodeItem<_KeyType, _LockType>,
			_STLKeyHasher,
			true> // Enable streaming compression of WALs
	{
	public:
		typedef Node<_KeyType, LockNodeRequest<_KeyType, _LockType>, LockNodeItem<_KeyType, _LockType>, _STLKeyHasher, true> NodeBase;

		struct Config
		{
			NodeConfig		m_node;
		};

		typedef LockNodeRequest<_KeyType, _LockType> Request;
		typedef LockNodeItem<_KeyType, _LockType> Item;

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

					aWriter->WriteItem(item);

					if (item->m_pendingWAL != NULL)
					{
						item->m_pendingWAL->RemoveReference();
						item->m_pendingWAL = NULL;
					}

					if(item->m_walInstanceCount > 0)
					{
						JELLY_ASSERT(item->m_walInstanceCount <= this->m_pendingStoreWALItemCount);
						this->m_pendingStoreWALItemCount -= item->m_walInstanceCount;
						item->m_walInstanceCount = 0;
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
		* -------|-----------------------------------------------------------------------------------------------
		* m_key  | Request key.
		* m_lock | Lock id that should be used.
		*
		* Output         | <!-- -->
		* ---------------|-----------------------------------------------------------------------------------
		* m_blobSeq      | Latest blob sequence number.
		* m_blobNodeIds  | Up to 4 ids of blob nodes where blob is stored.
		* m_lock         | Previous lock id.
		*/
		void
		Lock(
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);

			aRequest->m_callback = [=, this]()
			{
				aRequest->m_result = _Lock(aRequest);
			};

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits an unlock request to the queue.
		*
		* Input          | <!-- -->
		* ---------------|-----------------------------------------------------------------------------------------------
		* m_key          | Request key.
		* m_lock         | Lock id that should be used.
		* m_blobSeq      | Latest blob sequence number.
		* m_blobNodeIds  | Up to 4 ids of blob nodes where blob is stored
		*/
		void
		Unlock(
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);

			aRequest->m_callback = [=, this]()
			{
				aRequest->m_result = _Unlock(aRequest);
			};

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits an delete request to the queue.
		*
		* Input          | <!-- -->
		* ---------------|-----------------------------------------------------------------------------------------------
		* m_key          | Request key.
		*/
		void
		Delete(
			Request*											aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);

			aRequest->m_callback = [=, this]()
			{
				aRequest->m_result = _Delete(aRequest);
			};

			this->AddRequestToQueue(aRequest);			
		}

	private:
		
		Config			m_lockNodeConfig;

		Result
		_Lock(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->m_key, item))
			{
				// Never seen before, apply lock
				this->SetItem(aRequest->m_key, item = new Item(aRequest->m_key, aRequest->m_lock));
			}
			else
			{
				if(!item->m_lock.IsSet())
				{
					// Not locked, just apply lock
					item->m_lock = aRequest->m_lock;
					aRequest->m_lock.Clear();
					aRequest->m_blobNodeIds = item->m_meta.m_blobNodeIds;
				}
				else if(item->m_lock != aRequest->m_lock)
				{
					// Locked by someone else, fail
					aRequest->m_lock = item->m_lock;
					return RESULT_ALREADY_LOCKED;
				}
				else
				{
					// Same lock, no need to write anything
					aRequest->m_blobSeq = item->m_meta.m_blobSeq;
					aRequest->m_blobNodeIds = item->m_meta.m_blobNodeIds;
					aRequest->m_timeStamp = item->m_meta.m_timeStamp;
					return RESULT_OK;
				}
			}

			aRequest->m_blobSeq = item->m_meta.m_blobSeq;
			aRequest->m_blobNodeIds = item->m_meta.m_blobNodeIds;

			item->m_meta.m_timeStamp = aRequest->m_timeStamp;
			item->m_meta.m_seq++;

			item->m_tombstone.Clear();

			aRequest->m_hasPendingWrite = true;

			this->WriteToWAL(item, &aRequest->m_completed, &aRequest->m_result);

			return RESULT_OK;
		}

		Result
		_Unlock(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->m_key, item))
			{
				// Doesn't exist
				return RESULT_DOES_NOT_EXIST;
			}
			else if(!item->m_lock.IsSet())
			{
				// It wasn't locked, fail
				return RESULT_NOT_LOCKED;
			}
			else if(item->m_lock != aRequest->m_lock)
			{
				// Locked by someone else, fail
				return RESULT_ALREADY_LOCKED;
			}
			
			item->m_lock.Clear();

			item->m_meta.m_blobSeq = aRequest->m_blobSeq;
			item->m_meta.m_blobNodeIds = aRequest->m_blobNodeIds;
			item->m_meta.m_timeStamp = aRequest->m_timeStamp;
			item->m_meta.m_seq++;
	
			aRequest->m_hasPendingWrite = true;

			this->WriteToWAL(item, &aRequest->m_completed, &aRequest->m_result);

			return RESULT_OK;
		}

		Result
		_Delete(
			Request*											aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->m_key, item))
			{
				// Doesn't exist
				return RESULT_DOES_NOT_EXIST;
			}
			else if (item->m_lock.IsSet())
			{
				// It is locked, fail
				return RESULT_ALREADY_LOCKED;
			}

			item->m_meta.m_blobSeq = UINT32_MAX;
			item->m_meta.m_blobNodeIds = UINT32_MAX;
			item->m_meta.m_timeStamp = aRequest->m_timeStamp;
			item->m_meta.m_seq++;

			item->m_tombstone.Set(this->GetNextStoreId());

			aRequest->m_hasPendingWrite = true;

			this->WriteToWAL(item, &aRequest->m_completed, &aRequest->m_result);

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

				_KeyType key = item.get()->m_key;

				Item* existing;
				if (this->GetItem(key, existing))
				{
					if (item.get()->m_meta.m_seq > existing->m_meta.m_seq)
					{
						existing->MoveFrom(item.get());

						if (existing->m_pendingWAL != NULL)
						{
							existing->m_pendingWAL->RemoveReference();
							existing->m_pendingWAL = NULL;
						}
						else
						{
							this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(existing->m_key, existing));
						}

						existing->m_pendingWAL = aWAL;
						existing->m_pendingWAL->AddReference();
					}
				}
				else
				{
					item->m_pendingWAL = aWAL;
					item->m_pendingWAL->AddReference();

					this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(item->m_key, item.get()));

					this->SetItem(key, item.release());
				}
			}

			aWAL->SetSize(aReader->GetReadOffset());
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

				_KeyType key = item.get()->m_key;

				Item* existing;
				if (this->GetItem(key, existing))
				{
					if (item.get()->m_meta.m_seq > existing->m_meta.m_seq)
					{
						existing->MoveFrom(item.get());
					}
				}
				else
				{
					this->SetItem(key, item.release());
				}
			}
		}

	};

}