#include <locale.h>

#include <jelly/API.h>

#include "CSVOutput.h"

namespace jelly::Test::Sim
{

	CSVOutput::CSVOutput(
		const char*			aPath,
		IStats*				aStats)
		: m_started(false)
		, m_stats(aStats)
	{
		// Figure out decimal point character in current locale
		{
			setlocale(LC_ALL, ""); 
			const struct lconv* l = localeconv();
			JELLY_ASSERT(l != NULL);
			m_decimalPointCharacter = l->decimal_point[0];
			setlocale(LC_ALL, "C");
		}

		m_f = fopen(aPath, "wb");
		JELLY_CHECK(m_f != NULL, "fopen() failed: %s", aPath);
	}

	CSVOutput::~CSVOutput()
	{
		fclose(m_f);
	}

	void
	CSVOutput::AddColumn(
		const char*			aColumn)
	{	
		std::stringstream tokenizer(aColumn);
		std::string token;
		std::vector<std::string> tokens;
		while (std::getline(tokenizer, token, ':'))
			tokens.push_back(token);

		ColumnType columnType = COLUMN_TYPE_UNDEFINED;

		JELLY_CHECK(tokens.size() > 0, "CSV column syntax error: %s", aColumn);
		std::string statsString;
		
		char headerSuffix[256];

		if(tokens[0] == "s")
		{
			JELLY_CHECK(tokens.size() == 3, "CSV column syntax error: %s", aColumn);

			if(tokens[1] == "avg")
				columnType = COLUMN_TYPE_SAMPLE_AVG;
			else if (tokens[1] == "max")
				columnType = COLUMN_TYPE_SAMPLE_MAX;
			else if (tokens[1] == "min")
				columnType = COLUMN_TYPE_SAMPLE_MIN;

			snprintf(headerSuffix, sizeof(headerSuffix), " (%s)", tokens[1].c_str());

			statsString = tokens[2];
		}
		else if(tokens[0] == "g")
		{
			JELLY_CHECK(tokens.size() == 2, "CSV column syntax error: %s", aColumn);
			
			statsString = tokens[1];
			columnType = COLUMN_TYPE_GAUGE;

			headerSuffix[0] = '\0';
		}
		else if (tokens[0] == "c")
		{
			JELLY_CHECK(tokens.size() == 3, "CSV column syntax error: %s", aColumn);

			if (tokens[1] == "total")
				columnType = COLUMN_TYPE_COUNTER;
			else if (tokens[1] == "rate")
				columnType = COLUMN_TYPE_COUNTER_RATE;
			else if (tokens[1] == "rate_ma")
				columnType = COLUMN_TYPE_COUNTER_RATE_MA;

			snprintf(headerSuffix, sizeof(headerSuffix), " (%s)", tokens[1].c_str());

			statsString = tokens[2];
		}

		JELLY_CHECK(columnType != COLUMN_TYPE_UNDEFINED, "CSV column syntax error %s", aColumn);

		uint32_t id = m_stats->GetIdByString(statsString.c_str());
		JELLY_CHECK(id != UINT32_MAX, "Invalid statistics: %s", aColumn);
		m_columns.push_back({ columnType, statsString, headerSuffix, id });
	}

	void	
	CSVOutput::WriteRow()
	{
		if(!m_started)
		{
			// Write header with column names
			for(size_t i = 0; i < m_columns.size(); i++)
				fprintf(m_f, i == 0 ? "%s%s" : ";%s%s", m_columns[i].m_string.c_str(), m_columns[i].m_headerSuffix.c_str());

			fprintf(m_f, "\r\n");

			m_started = true;
		}
		
		for (size_t i = 0; i < m_columns.size(); i++)
		{
			float value = 0.0f;

			// Get value
			switch(m_columns[i].m_type)
			{
			case COLUMN_TYPE_SAMPLE_AVG:		value = (float)m_stats->GetSampler(m_columns[i].m_id).m_avg; break;
			case COLUMN_TYPE_SAMPLE_MIN:		value = (float)m_stats->GetSampler(m_columns[i].m_id).m_min; break;
			case COLUMN_TYPE_SAMPLE_MAX:		value = (float)m_stats->GetSampler(m_columns[i].m_id).m_max; break;
			case COLUMN_TYPE_GAUGE:				value = (float)m_stats->GetGauge(m_columns[i].m_id).m_value; break;
			case COLUMN_TYPE_COUNTER:			value = (float)m_stats->GetCounter(m_columns[i].m_id).m_value; break;
			case COLUMN_TYPE_COUNTER_RATE:		value = (float)m_stats->GetCounter(m_columns[i].m_id).m_rate; break;
			case COLUMN_TYPE_COUNTER_RATE_MA:	value = (float)m_stats->GetCounter(m_columns[i].m_id).m_rateMA; break;

			default:
				JELLY_ASSERT(false);
			}

			// Turn number into string - no trailing zeroes
			char buf[64];
			size_t len = (size_t)snprintf(buf, sizeof(buf), "%f", value);
			JELLY_ASSERT(len > 0 && len <= sizeof(buf));
			char* tail = &buf[len - 1];
			while(*tail == '0')
			{
				*tail = '\0';
				tail--;
			}

			if(*tail == '.')
				*tail = '\0';

			// Localized decimal point
			{
				len = strlen(buf);
				for(size_t j = 0; j < len; j++)
				{
					if(buf[j] == '.')
					{
						buf[j] = m_decimalPointCharacter;
						break;
					}
				}
			}

			fprintf(m_f, i == 0 ? "%s" : ";%s", buf);
		}

		fprintf(m_f, "\r\n");

		fflush(m_f);
	}

}