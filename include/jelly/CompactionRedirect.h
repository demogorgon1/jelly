#pragma once

namespace jelly
{

	// Compaction will remove a number of stores and create a new one. Each removed store
	// will need a CompactionRedirect object that specifies where items have been moved to.
	template <typename _KeyType, typename _STLKeyHasher>
	class CompactionRedirect
	{
	public:
		struct Entry
		{
			uint32_t	m_storeId;
			size_t		m_offset;
		};

		CompactionRedirect()
		{

		}

		~CompactionRedirect()
		{

		}

		void
		AddEntry(
			const _KeyType&			aKey,
			uint32_t				aStoreId,
			size_t					aOffset)
		{
			m_map[aKey] = Entry{aStoreId, aOffset};
		}

		bool
		GetEntry(
			const _KeyType&			aKey,
			Entry&					aOut) const
		{
			Map::const_iterator i = m_map.find(aKey);
			if(i == m_map.end())
				return false;

			aOut = i->second;
			return true;
		}

	private:

		typedef std::unordered_map<_KeyType, Entry, _STLKeyHasher> Map;
		Map				m_map;
	};

}