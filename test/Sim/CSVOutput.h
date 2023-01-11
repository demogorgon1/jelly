#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <stdio.h>

namespace jelly::Test::Sim
{

	class CSVOutput
	{
	public:
				CSVOutput(
					const char*			aPath);
				~CSVOutput();

		void	AddColumn(
					const char*			aColumn);
		void	StartNewRow();
		void	Flush();
		void	ShowColumn(
					const char*			aColumn);

		template <typename _T>
		void	
		SetColumn(
			const char*					aColumn,
			_T							aValue)
		{
			uint32_t i = _GetColumnIndex(aColumn);
			if(i == UINT32_MAX)
				return;

			Row* row = _GetCurrentRow();
			JELLY_ASSERT(row->m_columns.size() == m_columnNames.size());
			JELLY_ASSERT((size_t)i < m_columnNames.size());
			row->m_columns[i] = (float)aValue;
		}

	private:

		std::unordered_set<std::string>				m_showColumns;
		std::vector<std::string>					m_columnNames;
		std::unordered_map<std::string, uint32_t>	m_columnNameTable;
		char										m_decimalPointCharacter;

		struct Row
		{
			std::vector<float>						m_columns;
		};

		std::vector<Row*>							m_rows;
		FILE*										m_f;
		bool										m_started;

		uint32_t	_GetColumnIndex(
						const char*		aColumn);
		Row*		_GetCurrentRow();
	};

}