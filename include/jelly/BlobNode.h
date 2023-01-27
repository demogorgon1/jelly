#pragma once

#include "BlobNodeItem.h"
#include "BlobNodeRequest.h"
#include "Compaction.h"
#include "IFileStreamReader.h"
#include "IStoreBlobReader.h"
#include "List.h"
#include "Node.h" 
#include "Queue.h"

namespace jelly
{

	/**
	 * \brief A node for storing blobs.
	 * 
	 * \tparam _KeyType			Key type. For example \ref UIntKey.
	 * \tparam _BlobType		Blob type. For example \ref Blob.
	 */
	template 
	<
		typename _KeyType,
		typename _BlobType
	>
	class BlobNode
		: public Node<
			_KeyType, 
			BlobNodeRequest<_KeyType, _BlobType>, 
			BlobNodeItem<_KeyType, _BlobType>,
			typename _KeyType::Hasher,
			false> // Disable streaming compression of WALs (blobs are already compressed)
	{
	public:
		typedef Node<_KeyType, BlobNodeRequest<_KeyType, _BlobType>, BlobNodeItem<_KeyType, _BlobType>, typename _KeyType::Hasher, false> NodeBase;

		struct Config
		{
			Config()
				: m_maxResidentBlobSize(1024 * 1024 * 1024)
				, m_maxResidentBlobCount(1024 * 1024 * 1024)
			{

			}

			size_t			m_maxResidentBlobSize;
			size_t			m_maxResidentBlobCount;
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
			_InitStatsContext(&this->m_statsContext);

			_Restore();
			
			this->m_flushPendingStoreCallback = [&](
				uint32_t					aStoreId,
				IStoreWriter*				aWriter,
				NodeBase::PendingStoreType*	/*aPendingStoreType*/)
			{ 				
				size_t itemsProcessed = 0;

				for(std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

					runtimeState.m_storeId = aStoreId;
					runtimeState.m_storeOffset = aWriter->WriteItem(item);
					runtimeState.m_storeSize = item->GetBlob()->GetSize();

					if (runtimeState.m_pendingWAL != NULL)
					{
						runtimeState.m_pendingWAL->RemoveReference();
						runtimeState.m_pendingWAL = NULL;
					}

					if (runtimeState.m_walInstanceCount > 0)
					{
						JELLY_ASSERT(runtimeState.m_walInstanceCount <= this->m_pendingStoreWALItemCount);
						this->m_pendingStoreWALItemCount -= runtimeState.m_walInstanceCount;
						runtimeState.m_walInstanceCount = 0;
					}

					itemsProcessed++;
				}

				JELLY_ASSERT(this->m_pendingStoreWALItemCount == 0);

				_ObeyResidentBlobLimits();
 			};
		}

		~BlobNode()
		{
		}
		
		/**
		 * Submits a set request to the queue.
		 * 
		 * Input  | <!-- -->
		 * -------|-----------------------------------------------------------------------------------
		 * m_key  | Request key.
		 * m_seq  | Sequence number of the blob.
		 * m_blob | The blob.
		 * 
		 * Output | <!-- -->
		 * -------|-----------------------------------------------------------------------------------
		 * m_seq  | If RESULT_OUTDATED this returns the sequence number of the currently stored blob.
		 */
		void
		Set(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				aRequest->SetResult(_Update(aRequest, false)); // Update: Set
			});

			this->AddRequestToQueue(aRequest);
		}

  		 /**
		 * Submits a get request to the queue.
		 * 
		 * Input  | <!-- -->
		 * -------|-----------------------------------------------------------------------------------------------
		 * m_key  | Request key.
		 * m_seq  | Minimum sequence number of the blob (REUSLT_OUTDATED if sequence number isn't at least this).
		 * m_blob | The blob.
		 * 
		 * Output | <!-- -->
		 * -------|-----------------------------------------------------------------------------------
		 * m_blob | The blob.
		 * m_seq  | Returns the stored sequence number.
		 */
		void
		Get(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				aRequest->SetResult(_Get(aRequest));
			});

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits a delete request to the queue.
		*
		* Input  | <!-- -->
		* -------|-----------------------------------------------------------------------------------------------
		* m_key  | Request key.
		* m_seq  | Sequence number of the blob being deleted.
		*
		* Output | <!-- -->
		* -------|-----------------------------------------------------------------------------------
		* m_blob | The blob.
		* m_seq  | If RESULT_OUTDATED this returns the sequence number of the currently stored blob.
		*/
		void
		Delete(
			Request*										aRequest)
		{
			JELLY_ASSERT(aRequest->GetResult() == RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				aRequest->SetResult(_Update(aRequest, true)); // Update: Delete
			});

			this->AddRequestToQueue(aRequest);
		}

		// Testing: get keys of blobs stored in memory
		void
		GetResidentKeys(
			std::vector<_KeyType>&							aOut)
		{
			for (Item* item = m_residentItems.m_head; item != NULL; item = item->GetNext())
				aOut.push_back(item->GetKey());
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

			std::unique_ptr<BlobBuffer> newBlob;
			if(!aDelete)
			{
				newBlob = std::make_unique<BlobBuffer>();
				aRequest->GetBlob().ToBuffer(this->m_host->GetCompressionProvider(), *newBlob);
			}

			Item* item;
			if (!this->GetItem(aRequest->GetKey(), item))
			{
				if(aDelete)
				{
					return RESULT_DOES_NOT_EXIST;
				}
				else
				{
					this->SetItem(aRequest->GetKey(), item = new Item(aRequest->GetKey(), aRequest->GetSeq()));				

					item->SetBlob(newBlob.release());

					m_totalResidentBlobSize += item->GetBlob()->GetSize();
					m_residentItems.Add(item);

					obeyResidentBlobSizeLimit = true;
				}
			}
			else
			{
				if(item->GetSeq() >= aRequest->GetSeq())
				{
					// Trying to set an old version - return latest sequence number to requester
					aRequest->SetSeq(item->GetSeq());
					return RESULT_OUTDATED;
				}

				if(item->HasBlob())
				{
					// Already has a blob and is resident
					JELLY_ASSERT(m_totalResidentBlobSize >= item->GetBlob()->GetSize());
					m_totalResidentBlobSize -= item->GetBlob()->GetSize();

					if(!aDelete)
						m_totalResidentBlobSize += newBlob->GetSize();

					m_residentItems.MoveToTail(item);
				}
				else if(item->HasTombstone())
				{
					// Was deleted (and is also resident)
					if (!aDelete)
						m_totalResidentBlobSize += newBlob->GetSize();

					m_residentItems.MoveToTail(item);

					obeyResidentBlobSizeLimit = true;
				}
				else
				{
					// No blob, not resident
					if(!aDelete)
						m_totalResidentBlobSize += newBlob->GetSize();

					m_residentItems.Add(item);

					obeyResidentBlobSizeLimit = true;
				}

				if(aDelete)
					item->SetBlob(NULL);
				else
					item->SetBlob(newBlob.release());				
			}

			item->GetRuntimeState().m_isResident = true; 

			item->SetSeq(aRequest->GetSeq());
			item->SetTimeStamp(aRequest->GetTimeStamp());

			if(aDelete)
				item->SetTombstoneStoreId(this->GetNextStoreId());
			else
				item->RemoveTombstone();
			
			aRequest->WriteToWAL(this, item);

			if(obeyResidentBlobSizeLimit)
				_ObeyResidentBlobLimits();

			return RESULT_OK;
		}

		Result
		_Get(
			Request*										aRequest)
		{
			Item* item;
			if (!this->GetItem(aRequest->GetKey(), item))
				return RESULT_DOES_NOT_EXIST;

			if(item->HasTombstone())
				return RESULT_DOES_NOT_EXIST;

			if(item->GetSeq() < aRequest->GetSeq())
			{
				// Return stored sequence number
				aRequest->SetSeq(item->GetSeq());
				return RESULT_OUTDATED;
			}

			if(!item->HasBlob())
			{
				typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

				JELLY_ASSERT(runtimeState.m_storeId != UINT32_MAX);

				uint32_t storeId = runtimeState.m_storeId;
				size_t storeOffset = runtimeState.m_storeOffset;
				IStoreBlobReader* storeBlobReader = NULL;
				
				for(;;)
				{
					storeBlobReader = this->m_host->GetStoreBlobReader(this->m_nodeId, storeId, &this->m_statsContext.m_fileStore);
					if(storeBlobReader != NULL)
						break;

					// This store doesn't exist anymore, it was probably compacted away. Look up the compaction redirect.
					uint32_t newStoreId;
					if(!_GetCompactionRedirect(storeId, item->GetKey(), newStoreId, storeOffset))
					{
						JELLY_ASSERT(false);
					}

					storeId = newStoreId;
				}

				storeBlobReader->ReadItemBlob(storeOffset, item);

				m_totalResidentBlobSize += item->GetBlob()->GetSize();

				m_residentItems.Add(item);

				aRequest->GetBlob().FromBuffer(this->m_host->GetCompressionProvider(), *item->GetBlob());

				_ObeyResidentBlobLimits();
			}
			else
			{
				m_residentItems.MoveToTail(item);

				aRequest->GetBlob().FromBuffer(this->m_host->GetCompressionProvider(), *item->GetBlob());
			}

			aRequest->SetSeq(item->GetSeq());
			aRequest->SetTimeStamp(item->GetTimeStamp());

			return RESULT_OK;
		}

		void
		_InitStatsContext(
			NodeBase::StatsContext*		aStatsContext)
		{
			aStatsContext->m_fileWAL.m_idRead = Stat::ID_DISK_READ_BLOB_WAL_BYTES;
			aStatsContext->m_fileWAL.m_idWrite = Stat::ID_DISK_WRITE_BLOB_WAL_BYTES;

			aStatsContext->m_fileStore.m_idRead = Stat::ID_DISK_READ_BLOB_STORE_BYTES;
			aStatsContext->m_fileStore.m_idWrite = Stat::ID_DISK_WRITE_BLOB_STORE_BYTES;

			aStatsContext->m_idWALCount = Stat::ID_BLOB_WAL_COUNT;
			aStatsContext->m_idFlushPendingWALTime = Stat::ID_FLUSH_PENDING_BLOB_WAL_TIME;
			aStatsContext->m_idProcessRequestsTime = Stat::ID_PROCESS_BLOB_REQUESTS_TIME;
			aStatsContext->m_idFlushPendingStoreTime = Stat::ID_FLUSH_PENDING_BLOB_STORE_TIME;
			aStatsContext->m_idCleanupWALsTime = Stat::ID_CLEANUP_BLOB_WALS_TIME;
			aStatsContext->m_idPerformCompactionTime = Stat::ID_PERFORM_BLOB_COMPACTION_TIME;
			aStatsContext->m_idPerformMajorCompactionTime = Stat::ID_PERFORM_MAJOR_BLOB_COMPACTION_TIME;
			aStatsContext->m_idApplyCompactionTime = Stat::ID_APPLY_BLOB_COMPACTION_TIME;
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

				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadStoreStream(this->m_nodeId, id, &this->m_statsContext.m_fileStore));
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

					if (item->HasBlob())
					{
						timeStampSorter.insert(TimeStampSorterValue({ item->GetKey(), item->GetTimeStamp() }, item));

						totalSize += item->GetBlob()->GetSize();
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

				std::unique_ptr<IFileStreamReader> f(this->m_host->ReadWALStream(this->m_nodeId, id, false, &this->m_statsContext.m_fileWAL));
				if (f)
					_LoadWAL(f.get(), wal);

				if (wal->GetRefCount() > 0)
					this->m_nextWALId = id + 1;
			}

			this->CleanupWALs();

			_ObeyResidentBlobLimits();
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

				typename Item::RuntimeState& itemRuntimeState = item->GetRuntimeState();

				_KeyType key = item.get()->GetKey();

				Item* existing;
				if (this->GetItem(key, existing))
				{
					// We have an existing item for this key - update it if new sequence number is higher
					if (item->GetSeq() > existing->GetSeq())
					{
						if(existing->HasBlob())
						{
							JELLY_ASSERT(m_totalResidentBlobSize >= existing->GetBlob()->GetSize());

							m_totalResidentBlobSize -= existing->GetBlob()->GetSize();
						}

						if(item->HasBlob())
							m_totalResidentBlobSize += item->GetBlob()->GetSize();

						existing->MoveFrom(item.get());

						// Resident items sorted by age (newest at tail, which is this item now)
						if(itemRuntimeState.m_isResident)
							m_residentItems.MoveToTail(existing);

						typename Item::RuntimeState& existingRuntimeState = existing->GetRuntimeState();

						if (existingRuntimeState.m_pendingWAL != NULL)
						{
							// Item is already in pending store list, remove reference to current WAL as we'll add a new one
							existingRuntimeState.m_pendingWAL->RemoveReference();
							existingRuntimeState.m_pendingWAL = NULL;
						}
						else
						{
							// Item isn't in the pending store, add it
							this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(existing->GetKey(), existing));
						}

						existingRuntimeState.m_pendingWAL = aWAL;
						existingRuntimeState.m_pendingWAL->AddReference();
					}
				}
				else
				{
					// First time we see this key, add it
					m_residentItems.Add(item.get());

					itemRuntimeState.m_pendingWAL = aWAL;
					itemRuntimeState.m_pendingWAL->AddReference();

					this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(item->GetKey(), item.get()));

					if(item->HasBlob())
						m_totalResidentBlobSize += item->GetBlob()->GetSize();

					this->SetItem(key, item.release());
				}
			}

			aWAL->SetSize(aReader->GetTotalBytesRead());
		}

		void
		_LoadStore(
			IFileStreamReader*			aReader,
			uint32_t					aStoreId)
		{			
			while (!aReader->IsEnd())
			{
				std::unique_ptr<Item> item(new Item());

				typename Item::RuntimeState& itemRuntimeState = item->GetRuntimeState();

				itemRuntimeState.m_storeOffset = aReader->GetTotalBytesRead();
				itemRuntimeState.m_storeId = aStoreId;

				if(!item->Read(aReader, &itemRuntimeState.m_storeOffset))
					break;

				itemRuntimeState.m_storeSize = item->GetBlob()->GetSize();

				_KeyType key = item->GetKey();

				if(m_totalResidentBlobSize >= m_blobNodeConfig.m_maxResidentBlobSize || this->GetItemCount() >= m_blobNodeConfig.m_maxResidentBlobCount)
				{
					item->SetBlob(NULL);
					itemRuntimeState.m_isResident = false;
				}
				else
				{
					itemRuntimeState.m_isResident = true;
				}

				Item* existing;
				if (this->GetItem(key, existing))
				{
					if (item->GetSeq() > existing->GetSeq())
					{
						if(existing->HasBlob())
						{
							JELLY_ASSERT(m_totalResidentBlobSize >= existing->GetBlob()->GetSize());
							m_totalResidentBlobSize -= existing->GetBlob()->GetSize();
						}

						if(item->HasBlob())
							m_totalResidentBlobSize += item->GetBlob()->GetSize();

						existing->MoveFrom(item.get());
					}
				}
				else
				{
					if(item->HasBlob())
						m_totalResidentBlobSize += item->GetBlob()->GetSize();

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
		_ObeyResidentBlobLimits()
		{
			while(m_totalResidentBlobSize > m_blobNodeConfig.m_maxResidentBlobSize || m_residentItems.m_count > m_blobNodeConfig.m_maxResidentBlobCount)
			{
				Item* head = m_residentItems.m_head;
				JELLY_ASSERT(head != NULL);

				typename Item::RuntimeState& runtimeState = head->GetRuntimeState();

				JELLY_ASSERT(runtimeState.m_isResident);

				if(runtimeState.m_pendingWAL != NULL)
				{
					// We can't expell things that are currently in a WAL
					break;
				}

				JELLY_ASSERT(head->HasBlob());
				m_totalResidentBlobSize -= head->GetBlob()->GetSize();
				head->SetBlob(NULL);
				m_residentItems.Remove(head);

				runtimeState.m_isResident = false;
			}
		}

	};

}