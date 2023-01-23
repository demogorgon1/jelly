#pragma once

#include "CompactionRedirect.h"

namespace jelly
{
	
	// Holds the result of a compaction operation. PerformCompaction() (slow), which can run on any thread,
	// will put the result in this (files to be deleted and redirects), so it can then be applied
	// with ApplyCompactionResult() (fast) on the main thread.
	template <typename _KeyType, typename _STLKeyHasher>
	class CompactionResult
	{
	public:
		struct CompactedStore
		{
			CompactedStore(
				uint32_t										aStoreId,
				CompactionRedirect<_KeyType, _STLKeyHasher>*	aRedirect)
				: m_storeId(aStoreId)
				, m_redirect(aRedirect)
			{

			}

			uint32_t														m_storeId;
			std::unique_ptr<CompactionRedirect<_KeyType, _STLKeyHasher>>	m_redirect;
		};

		CompactionResult()
			: m_isMajorCompaction(false)
		{

		}

		~CompactionResult()
		{
			for(CompactedStore* t : m_compactedStores)
				delete t;
		}

		void
		AddCompactedStore(
			uint32_t											aStoreId,
			CompactionRedirect<_KeyType, _STLKeyHasher>*		aRedirect)
		{
			m_compactedStores.push_back(new CompactedStore(aStoreId, aRedirect));
		}

		void
		SetMajorCompaction(
			bool												aMajorCompaction)
		{
			m_isMajorCompaction = aMajorCompaction;
		}

		// Data access
		std::vector<CompactedStore*>&		GetCompactedStores() { return m_compactedStores; }
		bool								IsMajorCompaction() const { return m_isMajorCompaction; }
		
	private:
		
		std::vector<CompactedStore*>										m_compactedStores;
		bool																m_isMajorCompaction;
	};

}