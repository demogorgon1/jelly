#pragma once

#include <jelly/CompactionJob.h>
#include <jelly/IHost.h>

namespace jelly
{

	class ICompactionStrategy
	{
	public:
		struct TotalStoreSize
		{
			TotalStoreSize()
				: m_c(0)
				, m_dc(0)
				, m_ddc(0)
			{

			}

			size_t											m_c;	// Current
			int32_t											m_dc;	// Trend of m_c
			int32_t											m_ddc;	// Trend of m_dc;
		};

		typedef std::function<void(const CompactionJob&)> SuggestionCallback;

		ICompactionStrategy()
		{

		}

		virtual			
		~ICompactionStrategy() 
		{
		}

		// Virtual interface
		virtual void	 Update(
							const std::vector<IHost::StoreInfo>&	aStoreInfo,
							const TotalStoreSize&					aTotalStoreSize,
							SuggestionCallback						aSuggestionCallback) = 0;

	};

}