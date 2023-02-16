#include <jelly/API.h>

#include <windows.h>
#include <Psapi.h>

namespace jelly
{

	namespace Test
	{

		namespace
		{

			size_t g_testItemCount = 0;

			struct TestItem
			{
				TestItem(
					uint32_t	aKey = 0,
					uint32_t	aSeq = 0,
					uint32_t	aValue = 0)
					: m_key(aKey)
					, m_seq(aSeq)
					, m_value(aValue)
				{
					g_testItemCount++;
				}

				~TestItem()
				{
					JELLY_ASSERT(g_testItemCount > 0);
					g_testItemCount--;
				}

				const UIntKey<uint32_t>&
				GetKey() const
				{
					return m_key;
				}

				void
				Reset()
				{
					m_key = UIntKey<uint32_t>();
				}

				// Public data
				UIntKey<uint32_t>	m_key;
				uint32_t			m_seq;
				uint32_t			m_value;
			};

		}

		namespace ItemHashTableTest
		{

			void		
			Run()
			{	
				// Write a bunch of item to an ItemHashTable and an std::unordered_map and verify 
				// that they're identical at the end
				{
					JELLY_ASSERT(g_testItemCount == 0);

					std::mt19937 random;
					std::vector<std::pair<uint32_t, uint32_t>> testItems;
					for(size_t i = 0; i < 1000; i++)
						testItems.push_back({ random() % 200, random() % 1000 });

					ItemHashTable<UIntKey<uint32_t>, TestItem> table;
					std::unordered_map<uint32_t, uint32_t> tableRef;
	
					for(size_t i = 0; i < testItems.size(); i++)
					{
						uint32_t key = testItems[i].first;
						uint32_t value = testItems[i].second;

						std::pair<TestItem*, bool> result = table.InsertOrUpdate(key, key * 2);
						if(result.second)
						{
							// Inserted
							JELLY_ASSERT(result.first->m_value == 0);
							JELLY_ASSERT(result.first->m_seq == key * 2);
							JELLY_ASSERT(result.first->m_key == key);
							result.first->m_value = value;
						}
						else
						{
							// Updated
							JELLY_ASSERT(result.first->m_key == key);
							result.first->m_value = value;
							result.first->m_seq = key * 2;
						}

						tableRef[key] = value;
					}

					JELLY_ASSERT(tableRef.size() == table.Count());
					JELLY_ASSERT(g_testItemCount == tableRef.size());

					for(std::unordered_map<uint32_t, uint32_t>::const_iterator it = tableRef.begin(); it != tableRef.end(); it++)
					{
						const TestItem* t = table.Get(it->first);
						JELLY_ASSERT(t != NULL);
						JELLY_ASSERT(t->m_value == it->second);
					}

					table.ForEach([&tableRef](
						const TestItem* aItem)
					{
						std::unordered_map<uint32_t, uint32_t>::const_iterator it = tableRef.find(aItem->m_key.m_value);
						JELLY_ASSERT(it != tableRef.end());
						JELLY_ASSERT(aItem->m_value == it->second);
					});
					
					table.Clear();

					JELLY_ASSERT(g_testItemCount == 0);
				}
			}
		}

	}

}
