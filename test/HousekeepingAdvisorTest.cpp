#include <jelly/API.h>

#include "MemoryHost.h"
#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{

		namespace
		{

			typedef BlobNode<UIntKey<uint32_t>,	UInt32Blob,	UIntKey<uint32_t>::Hasher> BlobNodeType;
			typedef HousekeepingAdvisor<BlobNodeType> HousekeepingAdvisorType;

			void
			_UpdateHousekeepingAdvisor(
				HousekeepingAdvisorType&							aHousekeepingAdvisor,
				std::vector<HousekeepingAdvisorType::EventHandler>	aExpectedEvents)
			{
				size_t numEvents = 0;

				aHousekeepingAdvisor.Update([&](
					const HousekeepingAdvisorType::Event& aEvent)
				{
					JELLY_ASSERT(numEvents < aExpectedEvents.size());
					aExpectedEvents[numEvents](aEvent);
					numEvents++;
				});

				JELLY_ASSERT(numEvents == aExpectedEvents.size());
			}

		}

		namespace HousekeepingAdvisorTest
		{

			void		
			Run()
			{	
				// Note that this test doesn't really cover that much of HousekeepingAdvisor, as it's hard to
				// exercise the parts of that rely on timers and having lots of data on disk. 
				// I feel SimTest provides amble testing of this.

				MemoryHost host;
				BlobNodeType blobNode(&host, 0);				

				HousekeepingAdvisorType::Config config;
				config.m_minWALFlushIntervalMS = 0; // Want an immediate WAL flush

				HousekeepingAdvisorType housekeepingAdvisor(&host, &blobNode, config);

				// No events expected 
				_UpdateHousekeepingAdvisor(housekeepingAdvisor, {});

				// Do a set
				{
					BlobNodeType::Request req;
					req.m_key = 1;
					req.m_seq = 1;
					req.m_blob = 123;
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
				}

				// Since we now have a pending request, advisor should suggest a "flush pending WAL"
				_UpdateHousekeepingAdvisor(housekeepingAdvisor, 
				{
					[](const HousekeepingAdvisorType::Event& aEvent) { JELLY_ASSERT(aEvent.m_type == HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_WAL); }
				});
			}

		}

	}

}