#pragma once

#include "BlobNodeItem.h"
#include "BlobNodeRequest.h"
#include "IFileStreamReader.h"
#include "IStoreBlobReader.h"
#include "List.h"
#include "Node.h" 
#include "Queue.h"

namespace jelly
{

	// A node for storing blobs
	template 
	<
		typename _KeyType,
		typename _BlobType,
		typename _STLKeyHasher
	>
	class BlobNode
		: public Node<
			_KeyType, 
			BlobNodeRequest<_KeyType, _BlobType>, 
			BlobNodeItem<_KeyType, _BlobType>,
			_STLKeyHasher>
	{
	public:
		typedef Node<_KeyType, BlobNodeRequest<_KeyType, _BlobType>, BlobNodeItem<_KeyType, _BlobType>, _STLKeyHasher> NodeBase;

		struct Config
		{
			Config()
				: m_maxResidentBlobSize(1024 * 1024 * 1024)
			{

			}

			size_t			m_maxResidentBlobSize;
			NodeConfig		m_node;
		};

		typedef BlobNodeRequest<_KeyType, _BlobType> Request;
		typedef BlobNodeItem<_KeyType, _BlobType> Item;

		BlobNode(
			IHost*												aHost,
			uint32_t											aNodeId,
			const Config&										aConfig = Config())
			: NodeBase(aHost, aNodeId, aConfig.m_node)
			, m_blobNodeConfig(aConfig)
			, m_totalResidentBlobSize(0)
		{
			_Restore();

			this->m_compactionCallback = [&](uint32_t aOldest, uint32_t a1, uint32_t a2, CompactionResult<_KeyType, _STLKeyHasher>* aOut) 
			{ 
				_PerformCompaction(aOldest, a1, a2, aOut); 
			};
			
			this->m_flushPendingStoreCallback = [&](
				uint32_t					aStoreId,
				IStoreWriter*				aWriter,
				NodeBase::PendingStoreType*	/*aPendingStoreType*/)
			{ 				
				for(std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					item->m_storeId = aStoreId;
					item->m_storeOffset = aWriter->WriteItem(item, this->m_host->GetCompressionProvider());

					if (item->m_pendingWAL != NULL)
					{
						item->m_pendingWAL->RemoveReference();
						item->m_pendingWAL = NULL;
					}
				}

				_ObeyResidentBlobSizeLimit();
 			};
		}

		~BlobNode()
		{
		}
		
		// Set a blob
		// 
		// In: 
		//   m_key			Request key
		//   m_seq			Sequence number of the blob
		//   m_blob			The blob
		// Out:
		//   m_seq			If RESULT_OUTDATED this returns the sequence number of the 
		//                  currently stored blob
		void
		Set(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);
			JELLY_ASSERT(aRequest->m_blob.IsSet());

			aRequest->m_callback = [=]()
			{
				aRequest->m_result = _Update(aRequest, false); // Update: Set
			};

			this->AddRequestToQueue(aRequest);
		}

		// Get a blob
		// 
		// In: 
		//   m_key			Request key
		//   m_seq			Minimum sequence number of the blob (REUSLT_OUTDATED if 
		//                  sequence number isn't at least this)
		// Out:
		//   m_blob			The blob
		//   m_seq			Returns the stored sequence number
		void
		Get(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);

			aRequest->m_callback = [=]()
			{
				aRequest->m_result = _Get(aRequest);
			};

			this->AddRequestToQueue(aRequest);
		}

		// Delete a blob
		//
		// In: 
		//   m_key			Request key
		//   m_seq			Sequence number of the blob being deleted
		// Out:
		//   m_seq			If RESULT_OUTDATED this returns the sequence number of the 
		//                  currently stored blob
		void
		Delete(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->m_result == RESULT_NONE);

			aRequest->m_callback = [=]()
			{
				aRequest->m_result = _Update(aRequest, true); // Update: Delete
			};

			this->AddRequestToQueue(aRequest);
		}

		// Testing: get keys of blobs stored in memory
		void
		GetResidentKeys(
			std::vector<_KeyType>&							aOut)
		{
			for (Item* item = m_residentItems.m_head; item != NULL; item = item->m_next)
				aOut.push_back(item->m_key);
		}

		// Testing: get size of all blobs stored in memory
		size_t
		GetTotalResidentBlobSize()
		{
			return m_totalResidentBlobSize;
		}

	private:

		Config									m_blobNodeConfig;

		size_t									m_totalResidentBlobSize;

		List<Item>								m_residentItems;

		Result
		_Update(
			Request*									aRequest,
			bool										aDelete)
		{
			bool obeyResidentBlobSizeLimit = false;

			Item* item;
			if (!this->GetItem(aRequest->m_key, item))
			{
				if(aDelete)
				{
					return RESULT_DOES_NOT_EXIST;
				}
				else
				{
					m_totalResidentBlobSize += aRequest->m_blob.GetSize();

					this->SetItem(aRequest->m_key, item = new Item(aRequest->m_key, aRequest->m_seq));				

					item->m_blob.Move(aRequest->m_blob);

					m_residentItems.Add(item);

					obeyResidentBlobSizeLimit = true;
				}
			}
			else
			{
				if(item->m_meta.m_seq >= aRequest->m_seq)
				{
					// Trying to set an old version - return latest sequence number to requester
					aRequest->m_seq = item->m_meta.m_seq;
					return RESULT_OUTDATED;
				}

				if(item->m_blob.IsSet())
				{
					JELLY_ASSERT(m_totalResidentBlobSize >= item->m_blob.GetSize());
					m_totalResidentBlobSize -= item->m_blob.GetSize();

					if(!aDelete)
						m_totalResidentBlobSize += aRequest->m_blob.GetSize();

					m_residentItems.MoveToTail(item);
				}
				else
				{
					if(!aDelete)
						m_totalResidentBlobSize += aRequest->m_blob.GetSize();

					m_residentItems.Add(item);

					obeyResidentBlobSizeLimit = true;
				}

				if(aDelete)
					item->m_blob.Delete();
				else
					item->m_blob.Move(aRequest->m_blob);
			}

			item->m_meta.m_seq = aRequest->m_seq;
			item->m_meta.m_timeStamp = aRequest->m_timeStamp;

			if(aDelete)
				item->m_tombstone.Set(this->GetNextStoreId());
			else
				item->m_tombstone.Clear();
				
			this->WriteToWAL(item, &aRequest->m_completed, &aRequest->m_result);

			aRequest->m_hasPendingWrite = true;

			if(obeyResidentBlobSizeLimit)
				_ObeyResidentBlobSizeLimit();

			return RESULT_OK;
		}

		Result
		_Get(
			Request*										aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->m_key, item))
				return RESULT_DOES_NOT_EXIST;

			if(item->m_tombstone.IsSet())
				return RESULT_DOES_NOT_EXIST;

			if(item->m_meta.m_seq < aRequest->m_seq)
			{
				// Return stored sequence number
				aRequest->m_seq = item->m_meta.m_seq;
				return RESULT_OUTDATED;
			}

			if(!item->m_blob.IsSet())
			{
				// FIXME: slooow
				JELLY_ASSERT(item->m_storeId != UINT32_MAX);
				JELLY_ASSERT(item->m_storeOffset != -1);

				uint32_t storeId = item->m_storeId;
				size_t storeOffset = item->m_storeOffset;
				IStoreBlobReader* storeBlobReader = NULL;
				
				for(;;)
				{
					storeBlobReader = this->m_host->GetStoreBlobReader(this->m_nodeId, storeId);
					if(storeBlobReader != NULL)
						break;

					// This store doesn't exist anymore, it was probably compacted away. Look up the compaction redirect.
					uint32_t newStoreId;
					if(!_GetCompactionRedirect(storeId, item->m_key, newStoreId, storeOffset))
					{
						JELLY_ASSERT(false);
					}

					storeId = newStoreId;
				}

				storeBlobReader->ReadItemBlob(storeOffset, item);

				m_totalResidentBlobSize += item->m_blob.GetSize();

				m_residentItems.Add(item);

				aRequest->m_blob.Copy(item->m_blob);

				_ObeyResidentBlobSizeLimit();
			}
			else
			{
				m_residentItems.MoveToTail(item);

				aRequest->m_blob.Copy(item->m_blob);
			}

			aRequest->m_seq = item->m_meta.m_seq;
			aRequest->m_timeStamp = item->m_meta.m_timeStamp;

			return RESULT_OK;
		}

		void
		_Restore()
		{
			std::vector<uint32_t> walIds;
			std::vector<uint32_t> storeIds;

			this->m_host->EnumerateFiles(this->m_nodeId, walIds, storeIds);

			// Set next store id to be +1 of the current highest one
			if(storeIds.size() > 0)
				this->SetNextStoreId(storeIds[storeIds.size() - 1] + 1);

			// Load stores in reverse order, newest first. This means that if we hit the memory limit while loading, we'll probably 
			// have the newer data in memory.
			for (std::vector<uint32_t>::reverse_iterator i = storeIds.rbegin(); i != storeIds.rend(); i++)
			{
				uint32_t id = *i;

				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadStoreStream(this->m_nodeId, id));
				if (f)
					_LoadStore(f.get(), id);
			}
			
			// All stores loaded into memory, sort items by timestamp
			{
				struct TimeStampedKey
				{
					_KeyType	m_key;
					uint64_t	m_timeStamp;

					bool
					operator<(
						const TimeStampedKey& aOther) const
					{
						if (m_timeStamp == aOther.m_timeStamp)
							return m_key < aOther.m_key;

						return m_timeStamp < aOther.m_timeStamp;
					}
				};

				typedef std::map<TimeStampedKey, Item*> TimeStampSorter;
				typedef std::pair<const TimeStampedKey, Item*> TimeStampSorterValue;

				TimeStampSorter timeStampSorter;
				size_t totalSize = 0;

				for (std::pair<const _KeyType, Item*>& i : this->m_table)
				{
					Item* item = i.second;

					if (item->m_blob.IsSet())
					{
						timeStampSorter.insert(TimeStampSorterValue({ item->m_key, item->m_meta.m_timeStamp }, item));

						totalSize += item->m_blob.GetSize();
					}
				};

				JELLY_ASSERT(totalSize == m_totalResidentBlobSize);
				JELLY_ASSERT(m_residentItems.IsEmpty());

				for (TimeStampSorterValue& value : timeStampSorter)
					m_residentItems.Add(value.second);
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

			_ObeyResidentBlobSizeLimit();
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
						if(existing->m_blob.IsSet())
						{
							JELLY_ASSERT(m_totalResidentBlobSize >= existing->m_blob.GetSize());
							m_totalResidentBlobSize -= existing->m_blob.GetSize();
						}

						m_totalResidentBlobSize += item->m_blob.GetSize();

						existing->CopyFrom(item.get());

						m_residentItems.MoveToTail(existing);

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
					m_residentItems.Add(item.get());

					item->m_pendingWAL = aWAL;
					item->m_pendingWAL->AddReference();

					this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(item->m_key, item.get()));

					m_totalResidentBlobSize += item->m_blob.GetSize();

					this->SetItem(key, item.release());
				}
			}
		}

		void
		_LoadStore(
			IFileStreamReader*			aReader,
			uint32_t					aStoreId)
		{
			while (!aReader->IsEnd())
			{
				std::unique_ptr<Item> item(new Item());

				item->m_storeOffset = aReader->GetReadOffset();
				item->m_storeId = aStoreId;

				if(!item->Read(aReader, this->m_host->GetCompressionProvider()))
					break;

				_KeyType key = item.get()->m_key;

				if(m_totalResidentBlobSize >= m_blobNodeConfig.m_maxResidentBlobSize)
					item->m_blob.Reset();

				Item* existing;
				if (this->GetItem(key, existing))
				{
					if (item.get()->m_meta.m_seq > existing->m_meta.m_seq)
					{
						if(existing->m_blob.IsSet())
						{
							JELLY_ASSERT(m_totalResidentBlobSize >= existing->m_blob.GetSize());
							m_totalResidentBlobSize -= existing->m_blob.GetSize();
						}

						if(item->m_blob.IsSet())
							m_totalResidentBlobSize += item->m_blob.GetSize();

						existing->CopyFrom(item.get());
					}
				}
				else
				{
					if(item->m_blob.IsSet())
						m_totalResidentBlobSize += item->m_blob.GetSize();

					this->SetItem(key, item.release());
				}
			}
		}

		bool
		_GetCompactionRedirect(
			uint32_t				aStoreId,
			const _KeyType&			aKey,
			uint32_t&				aOutStoreId,
			size_t&					aOutOffset)
		{
			auto i = this-> m_compactionRedirectMap.find(aStoreId);
			if(i == this->m_compactionRedirectMap.end())
				return false;

			typename NodeBase::CompactionRedirectType::Entry t;
			if(!i->second->GetEntry(aKey, t))
				return false;

			aOutStoreId = t.m_storeId;
			aOutOffset = t.m_offset;
			return true;
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

			std::unique_ptr<NodeBase::CompactionRedirectType> compactionRedirect1(new NodeBase::CompactionRedirectType());
			std::unique_ptr<NodeBase::CompactionRedirectType> compactionRedirect2(new NodeBase::CompactionRedirectType());

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
					// FIXME: we don't need to decompress blobs for this

					if (!hasItem1)
					{
						item1.m_storeOffset = f1->GetReadOffset();
						hasItem1 = item1.Read(f1.get(), this->m_host->GetCompressionProvider());
					}

					if (!hasItem2)
					{
						item2.m_storeOffset = f2->GetReadOffset();
						hasItem2 = item2.Read(f2.get(), this->m_host->GetCompressionProvider());
					}

					if (!hasItem1 && !hasItem2)
						break;

					if (hasItem1 && !hasItem2)
					{
						if(!item1.m_tombstone.ShouldPrune(aOldestStoreId))
						{
							item1.m_storeOffset = fOut->WriteItem(&item1, this->m_host->GetCompressionProvider());
							compactionRedirect1->AddEntry(item1.m_key, newStoreId, item1.m_storeOffset);
						}

						hasItem1 = false;
					}
					else if (!hasItem1 && hasItem2)
					{
						if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
						{
							item2.m_storeOffset = fOut->WriteItem(&item2, this->m_host->GetCompressionProvider());
							compactionRedirect2->AddEntry(item2.m_key, newStoreId, item2.m_storeOffset);
						}

						hasItem2 = false;
					}
					else
					{
						JELLY_ASSERT(hasItem1 && hasItem2);

						if (item1.m_key < item2.m_key)
						{								
							if (!item1.m_tombstone.ShouldPrune(aOldestStoreId))
							{
								item1.m_storeOffset = fOut->WriteItem(&item1, this->m_host->GetCompressionProvider());
								compactionRedirect1->AddEntry(item1.m_key, newStoreId, item1.m_storeOffset);
							}

							hasItem1 = false;
						}
						else if (item2.m_key < item1.m_key)
						{
							if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
							{
								item2.m_storeOffset = fOut->WriteItem(&item2, this->m_host->GetCompressionProvider());
								compactionRedirect2->AddEntry(item2.m_key, newStoreId, item2.m_storeOffset);
							}

							hasItem2 = false;
						}
						else
						{
							// Items are the same - keep the one with the highest sequence number
							size_t offset = UINT64_MAX;

							if (item1.m_meta.m_seq > item2.m_meta.m_seq)
							{
								if (!item1.m_tombstone.ShouldPrune(aOldestStoreId))
									offset = fOut->WriteItem(&item1, this->m_host->GetCompressionProvider());
							}
							else
							{
								if (!item2.m_tombstone.ShouldPrune(aOldestStoreId))
									offset = fOut->WriteItem(&item2, this->m_host->GetCompressionProvider());
							}

							if(offset != UINT64_MAX)
							{
								compactionRedirect1->AddEntry(item1.m_key, newStoreId, offset);
								compactionRedirect2->AddEntry(item2.m_key, newStoreId, offset);
							}

							hasItem1 = false;
							hasItem2 = false;
						}
					}
				}
			}

			aOut->AddCompactedStore(aStoreId1, compactionRedirect1.release());
			aOut->AddCompactedStore(aStoreId2, compactionRedirect2.release());
		}

		void
		_ObeyResidentBlobSizeLimit()
		{
			while(m_totalResidentBlobSize > m_blobNodeConfig.m_maxResidentBlobSize)
			{
				Item* head = m_residentItems.m_head;
				JELLY_ASSERT(head != NULL);

				if(head->m_pendingWAL != NULL)
				{
					// We can't expell things that are currently in a WAL
					break;
				}

				JELLY_ASSERT(head->m_blob.IsSet());
				m_totalResidentBlobSize -= head->m_blob.GetSize();
				head->m_blob.Reset();
				m_residentItems.Remove(head);
			}
		}

	};

}