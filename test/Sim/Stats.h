#pragma once

#include "CSVOutput.h"

namespace jelly::Test::Sim
{

	struct Stats
	{
		enum Type
		{
			TYPE_COUNTER,
			TYPE_SAMPLE
		};

		struct Entry
		{
			Entry()
				: m_count(0)
				, m_sum(0)
				, m_min(0)
				, m_max(0)
			{

			}

			void
			Sample(
				uint64_t		aValue)
			{
				JELLY_ASSERT(m_count < UINT64_MAX);
				JELLY_ASSERT(UINT64_MAX - m_sum >= aValue);
				m_count++;
				m_sum += aValue;
				m_min = std::min(m_min, aValue);
				m_max = std::max(m_max, aValue);
			}

			void
			Reset()
			{
				m_count = 0;
				m_sum = 0;
				m_min = 0;
				m_max = 0;
			}

			void
			Add(
				const Entry&	aOther)
			{
				if(aOther.m_count > 0)
				{
					JELLY_ASSERT(m_count + aOther.m_count < UINT64_MAX);
					JELLY_ASSERT(UINT64_MAX - m_sum >= aOther.m_sum);
					m_count += aOther.m_count;
					m_sum += aOther.m_sum;
					m_min = std::min(m_min, aOther.m_min);
					m_max = std::max(m_max, aOther.m_max);
				}
			}

			// Public data
			uint64_t		m_count;
			uint64_t		m_sum;
			uint64_t		m_min;
			uint64_t		m_max;
		};

		static void
		MakeCSVColumnName(
			char*								aBuffer,
			size_t								aBufferSize,
			const char*							aColumnPrefix,
			const char*							aName,
			const char*							aSuffix)
		{
			snprintf(aBuffer, aBufferSize, "%s_%s%s", aColumnPrefix, aName, aSuffix);
		}

		static void
		InitCSVColumn(
			const char*							aColumnPrefix,
			const char*							aName,
			CSVOutput*							aCSV)
		{
			char columnName[256];
			MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "");
			aCSV->AddColumn(columnName);
		}

		static void
		InitStateInfoCSV(
			const char*							aColumnPrefix,
			const char*							aName,
			CSVOutput*							aCSV)
		{
			// Object count in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "");
				aCSV->AddColumn(columnName);
			}

			// Current avg time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_cur_avg");
				aCSV->AddColumn(columnName);
			}

			// Current min time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_cur_min");
				aCSV->AddColumn(columnName);
			}

			// Current max time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_cur_max");
				aCSV->AddColumn(columnName);
			}

			// Total avg time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_avg");
				aCSV->AddColumn(columnName);
			}

			// Total min time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_min");
				aCSV->AddColumn(columnName);
			}

			// Total max time spent in state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_max");
				aCSV->AddColumn(columnName);
			}

			// Number of objects left this state
			{
				char columnName[256];
				MakeCSVColumnName(columnName, sizeof(columnName), aColumnPrefix, aName, "_count");
				aCSV->AddColumn(columnName);
			}
		}

		static void
		PrintStateInfo(
			const char*							aName,
			uint32_t							aState,
			const std::vector<Stats::Entry>&	aStateInfo,
			const Stats&						aStats, 
			uint32_t							aIndex,
			const char*							aCSVColumnPrefix,
			CSVOutput*							aCSV,
			const Config*						aConfig)
		{
			JELLY_ASSERT((size_t)aState < aStateInfo.size());
			const Entry& stateEntry = aStateInfo[aState];

			if(stateEntry.m_count > 0)
			{
				const Entry& timeSampleEntry = aStats.GetEntry(aIndex);
				
				char timingInfoString[256];
				if(timeSampleEntry.m_count > 0)
				{
					if (aConfig->m_simTestStdOut)
					{
						snprintf(timingInfoString, sizeof(timingInfoString), "(changes: avg:%ums, min:%ums, max:%ums, count:%u)",
							(uint32_t)(timeSampleEntry.m_sum / timeSampleEntry.m_count), 
							(uint32_t)timeSampleEntry.m_min,
							(uint32_t)timeSampleEntry.m_max,
							(uint32_t)timeSampleEntry.m_count);
					}

					if(aCSV != NULL)
					{
						{
							char columnName[256];
							MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_avg");
							aCSV->SetColumn(columnName, timeSampleEntry.m_sum / timeSampleEntry.m_count);
						}

						{
							char columnName[256];
							MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_min");
							aCSV->SetColumn(columnName, timeSampleEntry.m_min);
						}

						{
							char columnName[256];
							MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_max");
							aCSV->SetColumn(columnName, timeSampleEntry.m_max);
						}

						{
							char columnName[256];
							MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_count");
							aCSV->SetColumn(columnName, timeSampleEntry.m_count);
						}
					}
				}
				else
				{
					timingInfoString[0] = '\0';
				}

				if (aConfig->m_simTestStdOut)
				{
					printf("[STATE]%s : %u avg:%.1fs, min:%.1fs, max:%.1fs %s\n", 
						aName, 
						(uint32_t)stateEntry.m_count, 
						((float)stateEntry.m_sum / (float)stateEntry.m_count) / 1000.0f, 
						(float)stateEntry.m_min / 1000.0f, 
						(float)stateEntry.m_max / 1000.0f,
						timingInfoString);
				}

				if (aCSV != NULL)
				{
					{
						char columnName[256];
						MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "");
						aCSV->SetColumn(columnName, stateEntry.m_count);
					}

					{
						char columnName[256];
						MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_cur_avg");
						aCSV->SetColumn(columnName, ((float)stateEntry.m_sum / (float)stateEntry.m_count) / 1000.0f);
					}

					{
						char columnName[256];
						MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_cur_min");
						aCSV->SetColumn(columnName, (float)stateEntry.m_min / 1000.0f);
					}

					{
						char columnName[256];
						MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "_cur_max");
						aCSV->SetColumn(columnName, (float)stateEntry.m_max / 1000.0f);
					}
				}
			}
		}

		Stats()
		{

		}

		~Stats()
		{

		}

		void
		Init(
			uint32_t		aNumEntries)
		{
			m_entries.resize((size_t)aNumEntries);
		}

		void
		Sample(
			uint32_t		aIndex,
			uint32_t		aValue = 0)
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			m_entries[aIndex].Sample((uint32_t)aValue);
		}

		void
		Reset()
		{
			for(Entry& t : m_entries)
				t.Reset();
		}

		void
		Add(
			const Stats&	aOther)
		{
			JELLY_ASSERT(m_entries.size() == aOther.m_entries.size());
			for(size_t i = 0; i < m_entries.size(); i++)
				m_entries[i].Add(aOther.m_entries[i]);
		}

		void
		AddAndResetEntry(
			uint32_t		aIndex,
			Entry&			aEntry)
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			m_entries[aIndex].Add(aEntry);
			aEntry.Reset();
		}

		const Entry&
		GetEntry(
			uint32_t		aIndex) const
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			return m_entries[aIndex];
		}

		void
		Print(
			Type			aType,
			uint32_t		aIndex,
			const char*		aName,
			const char*		aCSVColumnPrefix,
			CSVOutput*		aCSV,
			const Config*	aConfig) const
		{
			JELLY_ASSERT((size_t)aIndex < m_entries.size());
			const Entry& t = m_entries[aIndex];

			if(aType == TYPE_COUNTER)
			{
				if (aConfig->m_simTestStdOut)
					printf("%-32s : %u\n", aName, (uint32_t)t.m_count);

				if(aCSV != NULL)
				{
					char columnName[256];
					MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "");
					aCSV->SetColumn(columnName, t.m_count);
				}
			}
			else
			{
				if (t.m_count == 0)
				{
					if (aConfig->m_simTestStdOut)
						printf("%-32s : N/A\n", aName);
				}
				else
				{
					if (aConfig->m_simTestStdOut)
						printf("%-32s : [avg:%8u][min:%8u][max:%8u]\n", aName, (uint32_t)(t.m_sum / t.m_count), (uint32_t)t.m_min, (uint32_t)t.m_max);

					if (aCSV != NULL)
					{
						char columnName[256];
						MakeCSVColumnName(columnName, sizeof(columnName), aCSVColumnPrefix, aName, "");
						aCSV->SetColumn(columnName, t.m_sum / t.m_count);
					}
				}
			}
		}

		// Public data
		std::vector<Entry>		m_entries;
	};

}