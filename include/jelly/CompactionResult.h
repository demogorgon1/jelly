#pragma once

namespace jelly
{
	
	/**
	* \brief Holds the result of a compaction operation. 
	* 
	* PerformCompaction() (slow), which can run on any thread,
	* will put the result in this (files to be deleted and items moved), so it can then be applied
	* with ApplyCompactionResult() (fast) on the main thread.
	* 
	* @code
	* // This can be done on any thread
	* std::unique_ptr<CompactionResult<_KeyType, _STLKeyHasher>> compactionResult(node->PerformCompaction(compactionJob));
	* ...
	* // This must happen on the main thread
	* node->ApplyCompactionResult(compactionResult.get());
	* @endcode
	* 
	* @see BlobNode
	* @see LockNode
	*/		
	template <typename _KeyType, typename _STLKeyHasher>
	class CompactionResult
	{
	public:
		struct Item
		{
			Item(
				const _KeyType&									aKey = _KeyType(),
				uint32_t										aSeq = 0,
				uint32_t										aStoreId = 0,
				size_t											aStoreOffset = 0)
				: m_key(aKey)
				, m_seq(aSeq)
				, m_storeId(aStoreId)
				, m_storeOffset(aStoreOffset)
			{

			}

			_KeyType														m_key;
			uint32_t														m_seq;
			uint32_t														m_storeId;
			size_t															m_storeOffset;
		};

		typedef std::unordered_map<_KeyType, Item, _STLKeyHasher> ItemMap;

		CompactionResult()
			: m_isMajorCompaction(false)
		{

		}

		~CompactionResult()
		{
		}

		void
		AddItem(
			const _KeyType&									aKey,
			uint32_t										aSeq,
			uint32_t										aStoreId,
			size_t											aStoreOffset)
		{
			m_items.push_back(Item(aKey, aSeq, aStoreId, aStoreOffset));
		}

		void
		SetStoreIds(
			const std::vector<uint32_t>&					aStoreIds)
		{
			m_storeIds = aStoreIds;
		}

		void
		SetMajorCompaction(
			bool												aMajorCompaction)
		{
			m_isMajorCompaction = aMajorCompaction;
		}

		// Data access
		bool								IsMajorCompaction() const { return m_isMajorCompaction; }
		const std::vector<Item>&			GetItems() const { return m_items; }
		const std::vector<uint32_t>&		GetStoreIds() const { return m_storeIds; }
		
	private:
		
		std::vector<Item>													m_items;
		std::vector<uint32_t>												m_storeIds;
		bool																m_isMajorCompaction;
	};

}