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
	 * \tparam _MetaType		Meta data type.
	 */
	template 
	<
		typename _KeyType,
		typename _MetaType = MetaData::Dummy
	>
	class BlobNode
		: public Node<
			_KeyType, 
			BlobNodeRequest<_KeyType, _MetaType>, 
			BlobNodeItem<_KeyType, _MetaType>,
			false> // Disable streaming compression of WALs (blobs are already compressed)
	{
	public:
		typedef Node<_KeyType, BlobNodeRequest<_KeyType, _MetaType>, BlobNodeItem<_KeyType, _MetaType>, false> NodeBase;

		typedef BlobNodeRequest<_KeyType, _MetaType> Request;
		typedef BlobNodeItem<_KeyType, _MetaType> Item;

		BlobNode(
			IHost*												aHost,
			uint32_t											aNodeId)
			: NodeBase(aHost, aNodeId)
			, m_totalResidentBlobSize(0)
		{
			JELLY_CONTEXT(Exception::CONTEXT_BLOB_NODE_INIT);

			_InitStatsContext(&this->m_statsContext);

			_Restore();

			this->m_writePendingStoreCallback = [&](
				IStoreWriter*									aWriter,
				std::vector<size_t>&							aOutNewOffsets)
			{ 				
				aOutNewOffsets.resize(this->m_pendingStore.size());

				size_t index = 0;

				for(std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					aOutNewOffsets[index++] = aWriter->WriteItem(item);
				}
 			};

			this->m_finishPendingStoreCallback = [&](
				uint32_t										aStoreId,
				const std::vector<size_t>&						aNewOffsets)
			{ 				
				JELLY_ASSERT(aNewOffsets.size() == this->m_pendingStore.size());

				size_t index = 0;

				for(std::pair<const _KeyType, Item*>& i : this->m_pendingStore)
				{
					Item* item = i.second;

					typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

					runtimeState.m_storeId = aStoreId;
					runtimeState.m_storeOffset = aNewOffsets[index++];
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
				}

				JELLY_ASSERT(this->m_pendingStoreWALItemCount == 0);

				_ObeyResidentBlobLimits();
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
		~BlobNode()
		{
		}
		
		/**
		 * Submits a set request to the queue.
		 * 
		 * Input     | <!-- -->
		 * ----------|---------------------------------------------------------------------------------------
		 * m_key     | Request key.
		 * m_seq     | Sequence number of the blob.
		 * m_blob    | The blob.
		 * m_lockSeq | Lock sequence number.
		 * 
		 * Output    | <!-- -->
		 * ----------|---------------------------------------------------------------------------------------
		 * m_seq     | If REQUEST_RESULT_OUTDATED this returns the sequence number of the currently stored blob.
		 */
		void
		Set(
			Request*										aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_BLOB_NODE_SET);

				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_BLOB_SET_TIME);

				aRequest->SetResult(_Update(aRequest, false)); // Update: Set
			});

			this->AddRequestToQueue(aRequest);
		}

  		 /**
		 * Submits a get request to the queue.
		 * 
		 * Input     | <!-- -->
		 * ----------|-----------------------------------------------------------------------------------------------
		 * m_key     | Request key.
		 * m_seq     | Minimum sequence number of the blob (REQUEST_RESULT_OUTDATED if sequence number isn't at least this).
		 * m_blob    | The blob.
		 * m_lockSeq | Lock sequence number.
		 *
		 * Output    | <!-- -->
		 * ----------|-----------------------------------------------------------------------------------------------
		 * m_blob    | The blob.
		 * m_seq     | Returns the stored sequence number.
		 */
		void
		Get(
			Request*										aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_BLOB_NODE_GET);
					
				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_BLOB_GET_TIME);

				aRequest->SetResult(_Get(aRequest));
			});

			this->AddRequestToQueue(aRequest);
		}

		/**
		* Submits a delete request to the queue.
		*
		* Input     | <!-- -->
		* ----------|-----------------------------------------------------------------------------------------------
		* m_key     | Request key.
		* m_seq     | Sequence number of the blob being deleted.
		* m_lockSeq | Lock sequence number.
		*
		* Output    | <!-- -->
		* ----------|-----------------------------------------------------------------------------------------------
		* m_blob    | The blob.
		* m_seq     | If REQUEST_RESULT_OUTDATED this returns the sequence number of the currently stored blob.
		*/
		void
		Delete(
			Request*										aRequest) noexcept
		{
			JELLY_ASSERT(aRequest->GetResult() == REQUEST_RESULT_NONE);

			aRequest->SetExecutionCallback([=, this]()
			{
				JELLY_REQUEST_TYPE(Exception::REQUEST_TYPE_BLOB_NODE_DELETE);
				
				ScopedTimeSampler timerSampler(this->m_host->GetStats(), Stat::ID_BLOB_DELETE_TIME);

				aRequest->SetResult(_Update(aRequest, true)); // Update: Delete
			});

			this->AddRequestToQueue(aRequest);
		}

		// Testing: get keys of blobs stored in memory
		void
		GetResidentKeys(
			std::vector<_KeyType>&							aOut) noexcept
		{
			for (Item* item = m_residentItems.m_head; item != NULL; item = item->GetNext())
				aOut.push_back(item->GetKey());
		}

		// Testing: get size of all blobs stored in memory
		size_t
		GetTotalResidentBlobSize() noexcept
		{
			return m_totalResidentBlobSize;
		}

	private:

		size_t									m_totalResidentBlobSize;

		List<Item>								m_residentItems;

		RequestResult
		_Update(
			Request*									aRequest,
			bool										aDelete)
		{
			bool obeyResidentBlobSizeLimit = false;

			std::unique_ptr<IBuffer> newBlob;
			if(!aDelete)
			{
				size_t requestBlobSize = aRequest->GetBlob()->GetSize();

				const Compression::IProvider* compression = this->m_host->GetCompressionProvider();
				if(compression != NULL)
					newBlob.reset(compression->CompressBuffer(aRequest->GetBlob()));
				else
					newBlob.reset(aRequest->DetachBlob());

				this->m_host->GetStats()->Emit(Stat::ID_UNCOMPRESSED_BLOB_SIZE, requestBlobSize);
				this->m_host->GetStats()->Emit(Stat::ID_COMPRESSED_BLOB_SIZE, newBlob->GetSize());
			}

			std::pair<Item*, bool> result = this->m_table.InsertOrUpdate(aRequest->GetKey(), [aRequest, aDelete]()
			{
				if(aDelete)
					return (Item*)NULL;
				else
					return new Item(aRequest->GetKey(), aRequest->GetSeq());
			});

			Item* item = result.first;

			if(result.second)
			{
				// This is an insert.

				if(aDelete)
				{
					return REQUEST_RESULT_DOES_NOT_EXIST;
				}
				else
				{
					item->SetBlob(newBlob.release());

					m_totalResidentBlobSize += item->GetBlob()->GetSize();
					m_residentItems.Add(item);

					obeyResidentBlobSizeLimit = true;
				}
			}
			else
			{
				// This is an update.

				if(item->GetSeq() >= aRequest->GetSeq())
				{
					// Trying to set an old version - return latest sequence number to requester
					aRequest->SetSeq(item->GetSeq());
					return REQUEST_RESULT_OUTDATED;
				}

				if(aRequest->GetLockSeq().has_value() && item->GetLockSeq() != aRequest->GetLockSeq().value())
				{
					// We don't have the lock
					return REQUEST_RESULT_NOT_LOCKED;
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

			if(aRequest->GetMeta().has_value())
				item->SetMeta(aRequest->GetMeta().value());

			if(aDelete)
				item->SetTombstoneStoreId(this->GetNextStoreId());
			else
				item->RemoveTombstone();
			
			if(!aRequest->IsNoWrite())
			{
				aRequest->WriteToWAL(this, item);

				if (obeyResidentBlobSizeLimit)
					_ObeyResidentBlobLimits();
			}

			return REQUEST_RESULT_OK;
		}

		RequestResult
		_Get(
			Request*										aRequest)
		{
			Item* item = this->m_table.Get(aRequest->GetKey());
			if(item == NULL)
				return REQUEST_RESULT_DOES_NOT_EXIST;

			if(item->HasTombstone())
				return REQUEST_RESULT_DOES_NOT_EXIST;

			if(item->GetSeq() < aRequest->GetSeq())
			{
				// Return stored sequence number
				aRequest->SetSeq(item->GetSeq());
				return REQUEST_RESULT_OUTDATED;
			}

			if(!item->HasBlob())
			{
				typename Item::RuntimeState& runtimeState = item->GetRuntimeState();

				JELLY_ASSERT(runtimeState.m_storeId != UINT32_MAX);

				uint32_t storeId = runtimeState.m_storeId;
				size_t storeOffset = runtimeState.m_storeOffset;
				IStoreBlobReader* storeBlobReader = this->m_host->GetStoreBlobReader(this->m_nodeId, storeId, &this->m_statsContext.m_fileStore);
				JELLY_CHECK(storeBlobReader != NULL, Exception::ERROR_FAILED_TO_GET_BLOB_READER, "NodeId=%u;StoreId=%u", this->m_nodeId, storeId);

				storeBlobReader->ReadItemBlob(storeOffset, item);

				m_totalResidentBlobSize += item->GetBlob()->GetSize();

				m_residentItems.Add(item);

				Compression::IProvider* compression = this->m_host->GetCompressionProvider();
				if(compression != NULL)
					aRequest->SetBlob(compression->DecompressBuffer(item->GetBlob()));
				else
					aRequest->SetBlob(item->GetBlob()->Copy());

				_ObeyResidentBlobLimits();
			}
			else
			{
				m_residentItems.MoveToTail(item);

				Compression::IProvider* compression = this->m_host->GetCompressionProvider();
				if (compression != NULL)
					aRequest->SetBlob(compression->DecompressBuffer(item->GetBlob()));
				else
					aRequest->SetBlob(item->GetBlob()->Copy());
			}

			aRequest->SetSeq(item->GetSeq());
			aRequest->SetTimeStamp(item->GetTimeStamp());
			aRequest->SetMeta(item->GetMeta());
			
			if(aRequest->GetLockSeq().has_value())
				item->SetLockSeq(aRequest->GetLockSeq().value());

			return REQUEST_RESULT_OK;
		}

		void
		_ReplicateItem(
			Item&												aItem)
		{
			JELLY_ASSERT(this->m_replicationNetwork != NULL && !this->m_replicationNetwork->IsLocalNodeMaster());

			const _KeyType& key = aItem.GetKey();
			uint32_t seq = aItem.GetSeq();
			bool hasTombstone = aItem.HasTombstone();

			std::pair<Item*, bool> result = this->m_table.InsertOrUpdate(key, [key, seq, hasTombstone]() -> Item*
			{
				if(hasTombstone)
					return (Item*)NULL;
				else
					return new Item(key, seq);
			});

			if(result.first != NULL && (result.second || seq > result.first->GetSeq()))
			{
				JELLY_ASSERT(hasTombstone || aItem.HasBlob());
				Item* existing = result.first;

				if(existing->HasBlob())
				{
					JELLY_ASSERT(m_totalResidentBlobSize >= existing->GetBlob()->GetSize());
					m_totalResidentBlobSize -= existing->GetBlob()->GetSize();
					m_residentItems.MoveToTail(existing);
				}
				else
				{
					m_residentItems.Add(existing);
				}

				m_totalResidentBlobSize += aItem.GetBlob()->GetSize();

				existing->MoveFrom(&aItem);
				existing->GetRuntimeState().m_isResident = true;

				this->WriteToWAL(result.first, NULL, NULL, NULL);

				_ObeyResidentBlobLimits();
			}
		}

		void
		_InitStatsContext(
			NodeBase::StatsContext*		aStatsContext) noexcept
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
					uint64_t	m_timeStamp = 0;

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

				this->m_table.ForEach([&](
					Item* aItem) -> bool
				{
					if (aItem->HasBlob())
					{
						timeStampSorter.insert(TimeStampSorterValue({ aItem->GetKey(), aItem->GetTimeStamp() }, aItem));

						totalSize += aItem->GetBlob()->GetSize();
					}

					return true;
				});

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

				Item* existing = this->m_table.Get(key);
				if (existing != NULL)
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

						bool wasResident = existing->GetRuntimeState().m_isResident;

						existing->MoveFrom(item.get());

						// Resident items sorted by age (newest at tail, which is this item now)
						typename Item::RuntimeState& existingRuntimeState = existing->GetRuntimeState();

						if(wasResident)
							m_residentItems.MoveToTail(existing);
						else
							m_residentItems.Add(existing);

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

						existingRuntimeState.m_isResident = true;
						existingRuntimeState.m_pendingWAL = aWAL;
						existingRuntimeState.m_pendingWAL->AddReference();
					}
				}
				else
				{
					// First time we see this key, add it
					m_residentItems.Add(item.get());

					itemRuntimeState.m_isResident = true;
					itemRuntimeState.m_pendingWAL = aWAL;
					itemRuntimeState.m_pendingWAL->AddReference();

					this->m_pendingStore.insert(std::pair<const _KeyType, Item*>(item->GetKey(), item.get()));

					if(item->HasBlob())
						m_totalResidentBlobSize += item->GetBlob()->GetSize();

					this->m_table.Insert(key, item.release());
				}
			}

			aWAL->SetSize(aReader->GetTotalBytesRead());
		}

		void
		_LoadStore(
			IFileStreamReader*			aReader,
			uint32_t					aStoreId)
		{			
			size_t maxResidentBlobSize = this->m_config.GetSize(Config::ID_MAX_RESIDENT_BLOB_SIZE);
			size_t maxResidentBlobCount = this->m_config.GetSize(Config::ID_MAX_RESIDENT_BLOB_COUNT);

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

				if(m_totalResidentBlobSize >= maxResidentBlobSize || this->GetItemCount() >= maxResidentBlobCount)
				{
					item->SetBlob(NULL);
					itemRuntimeState.m_isResident = false;
				}
				else
				{
					itemRuntimeState.m_isResident = true;
				}

				Item* existing = this->m_table.Get(key);
				if (existing != NULL)
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

					this->m_table.Insert(key, item.release());
				}
			}
		}

		void
		_ObeyResidentBlobLimits()
		{
			ScopedTimeSampler timeSampler(this->m_host->GetStats(), Stat::ID_OBEY_RESIDENT_BLOB_LIMITS_TIME);

			size_t maxResidentBlobSize = this->m_config.GetSize(Config::ID_MAX_RESIDENT_BLOB_SIZE);
			size_t maxResidentBlobCount = this->m_config.GetSize(Config::ID_MAX_RESIDENT_BLOB_COUNT);

			while(m_totalResidentBlobSize > maxResidentBlobSize || m_residentItems.m_count > maxResidentBlobCount)
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

			this->m_host->GetStats()->Emit(Stat::ID_TOTAL_RESIDENT_BLOB_SIZE, m_totalResidentBlobSize);
			this->m_host->GetStats()->Emit(Stat::ID_TOTAL_RESIDENT_BLOB_COUNT, m_residentItems.m_count);
		}

	};

}