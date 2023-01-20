#pragma once

#include <unordered_set>

namespace jelly::Test::Sim
{

	class CSVOutput
	{
	public:
				CSVOutput(
					const char*			aPath,
					IStats*				aStats);
				~CSVOutput();

		void	AddColumn(
					const char*			aColumn);
		void	WriteRow();

	private:

		IStats*										m_stats;
		char										m_decimalPointCharacter;

		enum ColumnType
		{
			COLUMN_TYPE_UNDEFINED,

			COLUMN_TYPE_SAMPLE_AVG,
			COLUMN_TYPE_SAMPLE_MIN,
			COLUMN_TYPE_SAMPLE_MAX,
			COLUMN_TYPE_GAUGE,
			COLUMN_TYPE_COUNTER,
			COLUMN_TYPE_COUNTER_RATE,
			COLUMN_TYPE_COUNTER_RATE_MA
		};

		struct Column
		{
			ColumnType								m_type;
			std::string								m_string;
			std::string								m_headerSuffix;
			uint32_t								m_id;
		};
		
		std::vector<Column>							m_columns;
		FILE*										m_f;
		bool										m_started;
	};

}