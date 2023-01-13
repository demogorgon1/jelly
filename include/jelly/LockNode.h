#pragma once

#include "IFileStreamReader.h"
#include "LockNodeItem.h"
#include "LockNodeRequest.h"
#include "Node.h" 
#include "Queue.h"

namespace jelly
{

	// A node that keeps track of key locks and where the blobs are stored
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
			_STLKeyHasher>
	{
	public:
		typedef Node<_KeyType, LockNodeRequest<_KeyType, _LockType>, LockNodeItem<_KeyType, _LockType>, _STLKeyHasher> NodeBase;

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
			_Restore();

			this->m_compactionCallback = [&](
				uint32_t										aOldest, 
				uint32_t										a1, 
				uint32_t										a2, 
				CompactionResult<_KeyType, _STLKeyHasher>*		aOut) 
			{ 
				_PerformCompaction(aOldest, a1, a2, aOut); 
			};

			this->m_flushPendingStoreCallback = [&](
				uint32_t										/*aStoreId*/,
				IStoreWriter*									aWriter,
				NodeBase::PendingStoreType*						/*aPendingStoreType*/)
			{
				for (std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					aWriter->WriteItem(item, NULL);

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

		// Apply lock to a key
		// 
		// In: 
		//   m_key			Request key
		//   m_lock			Lock id that should be used
		// Out:
		//   m_blobSeq		Latest blob sequence number
		//   m_blobNodeIds	Up to 4 ids of blob nodes where blob is stored
		//   m_lock			Previous lock id
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

		// Unlock a key previously locked with the same lock id
		// 
		// In: 
		//   m_key			Request key
		//   m_lock			Lock id that should be used
		//   m_blobSeq		Latest blob sequence number
		//   m_blobNodeIds	Up to 4 ids of blob nodes where blob is stored
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

		// Deletes an unlocked key
		//
		// In:
		//   m_key			Request key
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
		_Restore()
		{
			std::vector<uint32_t> walIds;
			std::vector<uint32_t> storeIds;

			this->m_host->EnumerateFiles(this->m_nodeId, walIds, storeIds);

			for (uint32_t id : storeIds)
			{
				this->SetNextStoreId(id + 1);
				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadStoreStream(this->m_nodeId, id));
				if (f)
					_LoadStore(f.get(), id);
			}
			
			for (uint32_t id : walIds)
			{
				WAL* wal = this->AddWAL(id, NULL);

				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadWALStream(this->m_nodeId, id));
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
						existing->CopyFrom(item.get());

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
						existing->CopyFrom(item.get());
					}
				}
				else
				{
					this->SetItem(key, item.release());
				}
			}
		}


		void
		_PerformCompaction(
			uint32_t									aOldestStoreId,
			uint32_t									aStoreId1,
			uint32_t									aStoreId2,
			CompactionResult<_KeyType, _STLKeyHasher>*	aOut)
		{
			// Stores are always written in ascendening key order, so merging them is easy
			std::unique_ptr<IFileStreamReader> f1(this->m_host->ReadStoreStream(this->m_nodeId, aStoreId1));
			std::unique_ptr<IFileStreamReader> f2(this->m_host->ReadStoreStream(this->m_nodeId, aStoreId2));

			{
				uint32_t newStoreId = this->CreateStoreId();

				std::unique_ptr<IStoreWriter> fOut(this->m_host->CreateStore(this->m_nodeId, newStoreId));

				JELLY_ASSERT(f1 && f2 && fOut);

				Item item1;
				bool hasItem1 = false;

				Item item2;
				bool hasItem2 = false;

				for (;;)
				{
					if (!hasItem1)
						hasItem1 = item1.Read(f1.get(), NULL);

					if (!hasItem2)
						hasItem2 = item2.Read(f2.get(), NULL);

					if (!hasItem1 && !hasItem2)
						break;

					if (hasItem1 && !hasItem2)
					{
						if(!item1.m_tombstone.ShouldPrune(aOldestStoreId))
							fOut->WriteItem(&item1, NULL);

						hasItem1 = false;
					}
					else if (!hasItem1 && hasItem2)
					{
						if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
							fOut->WriteItem(&item2, NULL);

						hasItem2 = false;
					}
					else
					{
						JELLY_ASSERT(hasItem1 && hasItem2);

						if (item1.m_key < item2.m_key)
						{	
							if (!item1.m_tombstone.ShouldPrune(aOldestStoreId))
								fOut->WriteItem(&item1, NULL);

							hasItem1 = false;
						}
						else if (item2.m_key < item1.m_key)
						{
							if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
								fOut->WriteItem(&item2, NULL);

							hasItem2 = false;
						}
						else
						{
							// Items are the same - keep the one with the highest sequence number
							if (item1.m_meta.m_seq > item2.m_meta.m_seq)
							{
								if (!item1.m_tombstone.ShouldPrune(aOldestStoreId))
									fOut->WriteItem(&item1, NULL);
							}
							else
							{
								if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
									fOut->WriteItem(&item2, NULL);
							}

							hasItem1 = false;
							hasItem2 = false;
						}
					}
				}
			}

			aOut->AddCompactedStore(aStoreId1, NULL);
			aOut->AddCompactedStore(aStoreId2, NULL);
		}

	};

}