#include <jelly/API.h>

#include "MemoryHost.h"
#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{

		namespace
		{

			typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
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
				// I feel SimTest provides ample testing of this.

				MemoryHost host;
				host.GetDefaultConfigSource()->Set(jelly::Config::ID_MIN_WAL_FLUSH_INTERVAL_MS, "0");

				BlobNodeType blobNode(&host, 0);				

				HousekeepingAdvisorType housekeepingAdvisor(&host, &blobNode);

				// No events expected 
				_UpdateHousekeepingAdvisor(housekeepingAdvisor, {});

				// Do a set
				{
					BlobNodeType::Request req;
					req.SetKey(1);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(123));
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