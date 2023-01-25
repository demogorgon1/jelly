#pragma once

#include "CompactionJob.h"
#include "CompactionResult.h"
#include "IFileStreamReader.h"
#include "IHost.h"
#include "IStoreWriter.h"

namespace jelly
{

	namespace Compaction
	{

		// Do a "minor" compaction of just 2 stores
		template <typename _KeyType, typename _ItemType, typename _STLKeyHasher, typename _NodeType>
		void
		Perform(			
			_NodeType*									aNode,
			const CompactionJob&						aCompactionJob,
			CompactionResult<_KeyType, _STLKeyHasher>*	aOut)
		{
			// Stores are always written in ascendening key order, so merging them is easy
			std::unique_ptr<IFileStreamReader> f1(aNode->GetHost()->ReadStoreStream(aNode->GetNodeId(), aCompactionJob.m_storeId1, aNode->GetStoreFileStatsContext()));
			std::unique_ptr<IFileStreamReader> f2(aNode->GetHost()->ReadStoreStream(aNode->GetNodeId(), aCompactionJob.m_storeId2, aNode->GetStoreFileStatsContext()));

			std::unique_ptr<CompactionRedirect<_KeyType, _STLKeyHasher>> compactionRedirect1(new CompactionRedirect<_KeyType, _STLKeyHasher>());
			std::unique_ptr<CompactionRedirect<_KeyType, _STLKeyHasher>> compactionRedirect2(new CompactionRedirect<_KeyType, _STLKeyHasher>());

			{
				uint32_t newStoreId = aNode->CreateStoreId();

				std::unique_ptr<IStoreWriter> fOut(aNode->GetHost()->CreateStore(aNode->GetNodeId(), newStoreId, aNode->GetStoreFileStatsContext()));

				JELLY_ASSERT(f1 && f2 && fOut);

				_ItemType item1;
				bool hasItem1 = false;

				_ItemType item2;
				bool hasItem2 = false;

				for (;;)
				{
					if (!hasItem1)
						hasItem1 = item1.CompactionRead(f1.get());

					if (!hasItem2)
						hasItem2 = item2.CompactionRead(f2.get());

					if (!hasItem1 && !hasItem2)
						break;

					if (hasItem1 && !hasItem2)
					{
						uint64_t offset = item1.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());
						if(offset != UINT64_MAX)
							compactionRedirect1->AddEntry(item1.m_key, newStoreId, offset);

						hasItem1 = false;
					}
					else if (!hasItem1 && hasItem2)
					{
						uint64_t offset = item2.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());
						if (offset != UINT64_MAX)
							compactionRedirect2->AddEntry(item2.m_key, newStoreId, offset);

						hasItem2 = false;
					}
					else
					{
						JELLY_ASSERT(hasItem1 && hasItem2);

						if (item1.m_key < item2.m_key)
						{
							uint64_t offset = item1.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());
							if (offset != UINT64_MAX)
								compactionRedirect1->AddEntry(item1.m_key, newStoreId, offset);

							hasItem1 = false;
						}
						else if (item2.m_key < item1.m_key)
						{
							uint64_t offset = item2.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());
							if (offset != UINT64_MAX)
								compactionRedirect2->AddEntry(item2.m_key, newStoreId, offset);

							hasItem2 = false;
						}
						else
						{
							// Items are the same - keep the one with the highest sequence number
							size_t offset = UINT64_MAX;

							if (item1.m_meta.m_seq > item2.m_meta.m_seq)
								offset = item1.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());
							else
								offset = item2.CompactionWrite(aCompactionJob.m_oldestStoreId, fOut.get());

							if (offset != UINT64_MAX)
							{
								compactionRedirect1->AddEntry(item1.m_key, newStoreId, offset);
								compactionRedirect2->AddEntry(item2.m_key, newStoreId, offset);
							}

							hasItem1 = false;
							hasItem2 = false;
						}
					}
				}

				fOut->Flush();
			}

			aOut->AddCompactedStore(aCompactionJob.m_storeId1, compactionRedirect1.release());
			aOut->AddCompactedStore(aCompactionJob.m_storeId2, compactionRedirect2.release());
		}
		
		// Do a "major" compaction of everything. This is a more generalized version of PerformCompaction as 
		// it takes a list of stores instead of always 2. This is slightly slower though, hence having 2
		// different functions.
		template <typename _KeyType, typename _ItemType, typename _STLKeyHasher, typename _NodeType>
		void
		PerformMajorCompaction(
			_NodeType*									aNode,
			CompactionResult<_KeyType, _STLKeyHasher>*	aOut)
		{
			// Enumerate all stores
			std::vector<IHost::StoreInfo> storeInfo;
			aNode->GetHost()->GetStoreInfo(aNode->GetNodeId(), storeInfo);

			// Need at least 3 stores for compaction (can't touch the newest one)
			if (storeInfo.size() >= 3)
			{
				uint32_t oldestStoreId = storeInfo[0].m_id;

				struct SourceStore
				{
					SourceStore(
						uint32_t							aStoreId,
						IFileStreamReader* aFileStreamReader)
						: m_hasItem(false)
						, m_storeId(aStoreId)
						, m_fileStreamReader(aFileStreamReader)
						, m_redirect(new CompactionRedirect<_KeyType, _STLKeyHasher>())
					{
					}

					uint32_t														m_storeId;

					std::unique_ptr<IFileStreamReader>								m_fileStreamReader;
					std::unique_ptr<CompactionRedirect<_KeyType, _STLKeyHasher>>	m_redirect;

					_ItemType														m_item;
					bool															m_hasItem;
				};

				std::vector<std::unique_ptr<SourceStore>> sourceStores;

				// Open source stores
				for (size_t i = 0; i < storeInfo.size() - 1; i++)
				{
					uint32_t storeId = storeInfo[i].m_id;

					sourceStores.push_back(std::make_unique<SourceStore>(storeId, aNode->GetHost()->ReadStoreStream(aNode->GetNodeId(), storeId, aNode->GetStoreFileStatsContext())));

					JELLY_CHECK(sourceStores[sourceStores.size() - 1]->m_fileStreamReader, "Failed to open store for major compaction: %u", storeId);
				}

				// Open output store
				uint32_t newStoreId = aNode->CreateStoreId();
				std::unique_ptr<IStoreWriter> outputStore(aNode->GetHost()->CreateStore(aNode->GetNodeId(), newStoreId, aNode->GetStoreFileStatsContext()));
				JELLY_CHECK(outputStore, "Failed to open major compaction store for output: %u", newStoreId);

				// Perform the compaction
				std::vector<SourceStore*> lowestKeySourceStores;

				for (;;)
				{
					size_t numSourceStoresWithItemsRemaining = 0;

					for (std::unique_ptr<SourceStore>& sourceStore : sourceStores)
					{
						if (!sourceStore->m_hasItem && !sourceStore->m_fileStreamReader->IsEnd())
							sourceStore->m_hasItem = sourceStore->m_item.CompactionRead(sourceStore->m_fileStreamReader.get());

						if (sourceStore->m_hasItem)
							numSourceStoresWithItemsRemaining++;
					}

					if (numSourceStoresWithItemsRemaining == 0)
						break;

					// Find lowest key
					std::optional<_KeyType> lowestKey;

					for (std::unique_ptr<SourceStore>& sourceStore : sourceStores)
					{
						if (sourceStore->m_hasItem && (!lowestKey.has_value() || sourceStore->m_item.m_key < lowestKey))
							lowestKey = sourceStore->m_item.m_key;
					}

					JELLY_ASSERT(lowestKey.has_value());

					// Find stores with this key (might be multiple with equal or different sequence numbers)
					lowestKeySourceStores.clear();

					for (std::unique_ptr<SourceStore>& sourceStore : sourceStores)
					{
						if (sourceStore->m_hasItem && sourceStore->m_item.m_key == lowestKey)
							lowestKeySourceStores.push_back(sourceStore.get());
					}

					JELLY_ASSERT(lowestKeySourceStores.size() > 0);

					// Sort by sequence numbers - we want to use just the latest one
					std::sort(lowestKeySourceStores.begin(), lowestKeySourceStores.end(), [](
						const SourceStore* aLHS,
						const SourceStore* aRHS) -> bool
						{
							JELLY_ASSERT(aLHS->m_hasItem && aRHS->m_hasItem);
							JELLY_ASSERT(aLHS->m_item.m_key == aRHS->m_item.m_key);
							return aLHS->m_item.m_meta.m_seq > aRHS->m_item.m_meta.m_seq;
						});

					// Verify that array is ordered largest to smallest
					JELLY_ASSERT(lowestKeySourceStores[0]->m_item.m_meta.m_seq >= lowestKeySourceStores[lowestKeySourceStores.size() - 1]->m_item.m_meta.m_seq);

					{
						_ItemType& sourceItem = lowestKeySourceStores[0]->m_item;

						size_t offset = sourceItem.CompactionWrite(oldestStoreId, outputStore.get());

						for (SourceStore* lowestKeyStore : lowestKeySourceStores)
						{
							if (offset != UINT64_MAX)
								lowestKeyStore->m_redirect->AddEntry(lowestKey.value(), newStoreId, offset);

							JELLY_ASSERT(lowestKeyStore->m_hasItem);
							lowestKeyStore->m_hasItem = false;
						}
					}
				}

				// Finally add compacted store redirections to the output
				for (std::unique_ptr<SourceStore>& sourceStore : sourceStores)
					aOut->AddCompactedStore(sourceStore->m_storeId, sourceStore->m_redirect.release());
			}
		}

	}

}