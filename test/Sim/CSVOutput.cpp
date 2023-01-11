#include <locale.h>

#include <jelly/ErrorUtils.h>

#include "CSVOutput.h"

namespace jelly::Test::Sim
{

	CSVOutput::CSVOutput(
		const char*			aPath)
		: m_started(false)
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
		Flush();

		fclose(m_f);

		for(Row* t : m_rows)
			delete t;
	}

	void	
	CSVOutput::AddColumn(
		const char*			aColumn)
	{
		if(m_showColumns.find(aColumn) == m_showColumns.end())
			return;

		JELLY_ASSERT(m_rows.size() == 0);
		JELLY_ASSERT(_GetColumnIndex(aColumn) == UINT32_MAX);
		m_columnNames.push_back(aColumn);
		m_columnNameTable[aColumn] = (uint32_t)m_columnNames.size() - 1;
	}
	
	void	
	CSVOutput::StartNewRow()
	{
		m_rows.push_back(new Row());

		_GetCurrentRow()->m_columns.resize(m_columnNames.size(), 0.0f);
	}

	void	
	CSVOutput::Flush()
	{
		if(!m_started)
		{
			// Write header with column names
			for(size_t i = 0; i < m_columnNames.size(); i++)
				fprintf(m_f, i == 0 ? "%s" : ";%s", m_columnNames[i].c_str());			

			fprintf(m_f, "\r\n");

			m_started = true;
		}
		
		for(Row* t : m_rows)
		{
			JELLY_ASSERT(t->m_columns.size() == m_columnNames.size());

			for (size_t i = 0; i < t->m_columns.size(); i++)
			{
				// Turn number into string - no trailing zeroes
				char buf[64];
				size_t len = (size_t)snprintf(buf, sizeof(buf), "%f", t->m_columns[i]);
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

			delete t;
		}

		m_rows.clear();

		fflush(m_f);
	}

	void	
	CSVOutput::ShowColumn(
		const char*			aColumn)
	{
		m_showColumns.insert(aColumn);
	}

	//---------------------------------------------------------------------------------------

	uint32_t	
	CSVOutput::_GetColumnIndex(
		const char*			aColumn)
	{
		std::unordered_map<std::string, uint32_t>::iterator i = m_columnNameTable.find(aColumn);
		if(i == m_columnNameTable.end())
			return UINT32_MAX;
		return i->second;
	}

	CSVOutput::Row* 
	CSVOutput::_GetCurrentRow()
	{
		JELLY_ASSERT(m_rows.size() > 0);
		return m_rows[m_rows.size() - 1];
	}

}