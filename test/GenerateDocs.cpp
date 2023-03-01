#include <jelly/API.h>

namespace jelly::Test::GenerateDocs
{

	void		
	Run()
	{
		// Metrics
		{
			std::vector<const jelly::Stat::Info*> stats;
			for (uint32_t i = 0; i < jelly::Stat::GetCount(); i++)
				stats.push_back(jelly::Stat::GetInfo(i));

			std::sort(stats.begin(), stats.end(), [](
				const jelly::Stat::Info* aLHS,
				const jelly::Stat::Info* aRHS)
			{
				return strcmp(aLHS->m_id, aRHS->m_id) < 0;
			});

			printf("Id|String Id|Type\n");
			printf(":-|:-|:-\n");

			for(const jelly::Stat::Info* t : stats)
			{	
				// Convert string name to enum id
				{
					printf("ID_");
					size_t strLen = strlen(t->m_id);
					for(size_t i = 0; i < strLen; i++)
					{
						char c = t->m_id[i];
						if(c >= 'a' && c <= 'z')
							printf("%c", c - ('a' - 'A'));
						else
							printf("%c", c);
					}

					printf("|");
				}

				printf("%s|", t->m_id);

				switch(t->m_type)
				{
				case jelly::Stat::TYPE_COUNTER:	printf("Counter"); break;
				case jelly::Stat::TYPE_GAUGE:	printf("Gauge"); break;
				case jelly::Stat::TYPE_SAMPLER:	printf("Sampler"); break;
				default:						break;
				}

				printf("\n");
			}

			printf("\n");
		}

		// Configuration options
		{
			std::vector<const jelly::Config::Info*> options;
			for (uint32_t i = 0; i < jelly::Config::GetCount(); i++)
				options.push_back(jelly::Config::GetInfo(i));

			std::sort(options.begin(), options.end(), [](
				const jelly::Config::Info* aLHS,
				const jelly::Config::Info* aRHS)
			{
				return strcmp(aLHS->m_id, aRHS->m_id) < 0;
			});

			printf("Option|Type|Restart Required|Default|Description\n");
			printf(":-|:-|:-|:-|:-\n");

			for (const jelly::Config::Info* t : options)
			{
				printf("%s|", t->m_id);

				switch (t->m_type)
				{
				case jelly::Config::TYPE_UINT32:	printf("uint32|"); break;
				case jelly::Config::TYPE_SIZE:		printf("size|"); break;
				case jelly::Config::TYPE_INTERVAL:	printf("interval|"); break;
				case jelly::Config::TYPE_STRING:	printf("string|"); break;
				case jelly::Config::TYPE_BOOL:		printf("bool|"); break;
				default:							JELLY_ASSERT(false);
				}

				if (t->m_requiresRestart)
					printf("Yes|");
				else
					printf("No|");

				printf("%s|", t->m_default);

				size_t descLen = strlen(t->m_description);
				for (size_t j = 0; j < descLen; j++)
				{
					char c = t->m_description[j];
					if (c == '\"')
						printf("```");
					else
						printf("%c", c);
				}

				printf("\n");
			}

			printf("\n");
		}

		// Errors
		{
			std::vector<const jelly::Exception::ErrorInfo*> errors;
			for(uint32_t i = 1; i < (uint32_t)jelly::Exception::NUM_ERRORS; i++)
				errors.push_back(jelly::Exception::GetErrorInfo(i));

			std::sort(errors.begin(), errors.end(), [](
				const jelly::Exception::ErrorInfo* aLHS,
				const jelly::Exception::ErrorInfo* aRHS)
			{
				if(aLHS->m_category == aRHS->m_category)
					return strcmp(aLHS->m_string, aRHS->m_string) < 0;
				return (uint32_t)aLHS->m_category < (uint32_t)aRHS->m_category;
			});

			printf("Id|Category|Description\n");
			printf(":-|:-|:-\n");

			for(const jelly::Exception::ErrorInfo* t : errors)
			{
				printf("%s|%s|%s\n", t->m_string, jelly::Exception::GetCategoryString(t->m_category), t->m_description);
			}

			printf("\n");
		}
	}

}