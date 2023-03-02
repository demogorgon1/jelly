#pragma once

#include <jelly/CompactionJob.h>
#include <jelly/IHost.h>

namespace jelly
{

	// Abstract compaction strategy interface
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
			int32_t											m_dc;	// Derivative of m_c (velocity of total store size)
			int32_t											m_ddc;	// Derivative of m_dc (acceleration of total store size)
		};

		typedef std::function<void(const CompactionJob&)> SuggestionCallback;

		ICompactionStrategy() noexcept
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
							size_t									aAvailableDiskSpace,
							SuggestionCallback						aSuggestionCallback) = 0;

	};

}