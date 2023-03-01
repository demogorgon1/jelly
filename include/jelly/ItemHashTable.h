#pragma once

namespace jelly
{

	template <typename _KeyType, typename _ItemType>
	class ItemHashTable
	{
	public:
		struct TableEntry
		{
			_KeyType	m_key;
			_ItemType*	m_item = NULL;
		};

		ItemHashTable()
			: m_size(16)
			, m_count(0)
		{
			m_table = new TableEntry[m_size * 2];
		}

		~ItemHashTable()
		{
			Clear();
		}

		void
		Grow() noexcept
		{
			size_t newSize = m_size * 2;
			
			while(!_Rehash(newSize))
				newSize *= 2;
		}

		void
		Clear() noexcept
		{
			if(m_table != NULL)
			{
				TableEntry* entry = m_table;
				size_t tableArraySize = m_size * 2;
				for(size_t i = 0; i < tableArraySize; i++)
				{
					if(entry->m_item != NULL)
						delete entry->m_item;

					entry++;
				}

				delete [] m_table;

				m_size = 0;
				m_table = NULL;
				m_count = 0;
			}
		}

		std::pair<_ItemType*, bool>
		InsertOrUpdate(
			const _KeyType&				aKey,
			std::function<_ItemType*()>	aItemCreator) noexcept
		{
			uint64_t hash = aKey.GetHash();

			_ItemType* item = _TryGet(hash, aKey);
			if(item != NULL)
				return std::make_pair(item, false);

			item = aItemCreator();

			if(item != NULL)
			{
				TableEntry insert;
				insert.m_item = item;
				insert.m_key = aKey;

				while (!_TryInsert(m_table, m_size, insert, hash))
					Grow();

				m_count++;
			}

			return std::make_pair(item, true);
		}

		void
		Insert(
			const _KeyType&				aKey,
			_ItemType*					aItem) noexcept
		{
			uint64_t hash = aKey.GetHash();			
			JELLY_ASSERT(_TryGet(hash, aKey) == NULL);

			TableEntry insert;
			insert.m_item = aItem;
			insert.m_key = aKey;

			while (!_TryInsert(m_table, m_size, insert, hash))
				Grow();

			m_count++;
		}

		const _ItemType*
		Get(
			const _KeyType&				aKey) const noexcept
		{
			uint64_t hash = aKey.GetHash();

			return _TryGet(hash, aKey);
		}

		_ItemType*
		Get( 
			const _KeyType&				aKey) noexcept
		{
			uint64_t hash = aKey.GetHash();

			return _TryGet(hash, aKey);
		}

		uint32_t
		GetLoadFactor() const noexcept
		{
			if (m_size == 0)
				return 0;

			return (uint32_t)((100 * m_count) / (m_size * 2));
		}

		size_t
		Count() const noexcept
		{
			return m_count;
		}

		void
		ForEach(
			std::function<bool(const _ItemType*)>	aCallback) const
		{
			const TableEntry* entry = (const TableEntry*)m_table;
			size_t tableArraySize = m_size * 2;
			size_t found = 0;

			for (size_t i = 0; i < tableArraySize && found < m_count; i++)
			{
				if (entry->m_item != NULL)
				{
					if(!aCallback(entry->m_item))
						break;

					found++;
				}

				entry++;
			}
		}

		void
		ForEach(
			std::function<bool(_ItemType*)>			aCallback) 
		{
			TableEntry* entry = (TableEntry*)m_table;
			size_t tableArraySize = m_size * 2;
			size_t found = 0;

			for (size_t i = 0; i < tableArraySize && found < m_count; i++)
			{
				if (entry->m_item != NULL)
				{
					if(!aCallback(entry->m_item))
						break;

					found++;
				}

				entry++;
			}
		}

	private:

		size_t			m_count;
		size_t			m_size;
		TableEntry*		m_table;

		bool
		_TryInsert(
			TableEntry*		aTable,
			size_t			aSize,
			TableEntry&		aInsert,
			uint64_t&		aInsertHash)
		{	
			// Does it fit in one of the two nests?
			size_t i = aInsertHash % aSize;
			{
				TableEntry* entry1 = &aTable[i];
				if(entry1->m_item == NULL)
				{
					*entry1 = aInsert;
					return true;
				}

				TableEntry* entry2 = &aTable[aSize + (aInsertHash >> 32ULL) % aSize];
				if (entry2->m_item == NULL)
				{
					*entry2 = aInsert;
					return true;
				}
			}

			// Nope, we have to push things around
			{
				size_t level = 0;

				for (uint32_t loop = 0; loop < 16; loop++)
				{
					TableEntry next = aTable[i];
					aTable[i] = aInsert;

					if (next.m_item == NULL)
						return true;

					aInsert = next;
					aInsertHash = aInsert.m_key.GetHash();

					level = (level + 1) % 2;

					i = level * aSize + (aInsertHash >> (level << 5ULL)) % aSize;
				}
			}

			return false;
		}

		_ItemType*
		_TryGet(
			uint64_t		aHash,
			const _KeyType&	aKey)
		{
			TableEntry* entry1 = &m_table[aHash % m_size];
			if(entry1->m_key == aKey)
				return entry1->m_item;
				 
			TableEntry* entry2 = &m_table[m_size + (aHash >> 32ULL) % m_size];
			if (entry2->m_key == aKey)
				return entry2->m_item;

			return NULL;
		}

		const _ItemType*
		_TryGet(
			uint64_t		aHash,
			const _KeyType&	aKey) const
		{
			const TableEntry* entry1 = &m_table[aHash % m_size];
			if(entry1->m_key == aKey)
				return entry1->m_item;

			const TableEntry* entry2 = &m_table[m_size + (aHash >> 32ULL) % m_size];
			if (entry2->m_key == aKey)
				return entry2->m_item;

			return NULL;
		}

		bool
		_Rehash(
			size_t			aNewSize)
		{
			JELLY_ASSERT(aNewSize >= 1);
			JELLY_ASSERT(aNewSize > m_size);

			size_t newTableArraySize = aNewSize * 2;
			TableEntry* newTable = new TableEntry[newTableArraySize];

			if(m_table != NULL)
			{
				TableEntry* entry = m_table;
				size_t oldTableArraySize = m_size * 2;
				size_t found = 0;

				for (size_t i = 0; i < oldTableArraySize && found < m_count; i++)
				{
					if(entry->m_item != NULL)
					{
						uint64_t hash = entry->m_key.GetHash();

						if(!_TryInsert(newTable, aNewSize, *entry, hash))
						{
							delete [] newTable;
							return false;
						}

						found++;
					}

					entry++;
				}

				delete[] m_table;
			}

			m_size = aNewSize;
			m_table = newTable;

			return true;
		}

	};

}