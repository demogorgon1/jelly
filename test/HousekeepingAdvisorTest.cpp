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
					JELLY_ALWAYS_ASSERT(numEvents < aExpectedEvents.size());
					aExpectedEvents[numEvents](aEvent);
					numEvents++;
				});

				JELLY_ALWAYS_ASSERT(numEvents == aExpectedEvents.size());
			}

			void
			_TestLowPrioWALs()
			{
				// Different number of low-priority WALs than normal ones
				MemoryHost host;
				host.GetDefaultConfigSource()->Set(jelly::Config::ID_MIN_WAL_FLUSH_INTERVAL_MS, "0");
				host.GetDefaultConfigSource()->Set(jelly::Config::ID_MIN_LOW_PRIO_WAL_FLUSH_INTERVAL_MS, "0");
				host.GetDefaultConfigSource()->Set(jelly::Config::ID_WAL_CONCURRENCY, "1");
				host.GetDefaultConfigSource()->Set(jelly::Config::ID_WAL_CONCURRENCY_LOW_PRIO, "2");

				BlobNodeType blobNode(&host, 0);
				JELLY_ALWAYS_ASSERT(blobNode.GetPendingLowPrioWALCount() == 2);

				HousekeepingAdvisorType housekeepingAdvisor(&host, &blobNode);

				// Find a key that goes to the second low-priority WAL (same calculation as Node::WriteToWAL())
				uint32_t key = 1;
				while((2 * (UIntKey<uint32_t>(key).GetHash() >> 32)) / 0x100000000ULL != 1)
					key++;

				// Low-priority set
				BlobNodeType::Request req;
				req.SetKey(key);
				req.SetSeq(1);
				req.SetBlob(new UInt32Blob(123));
				req.SetLowPrio(true);
				blobNode.Set(&req);
				JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);
				JELLY_ALWAYS_ASSERT(blobNode.GetPendingLowPrioWALRequestCount(1) == 1);

				// Advisor should suggest flushing the second low-priority WAL
				_UpdateHousekeepingAdvisor(housekeepingAdvisor,
				{
					[](const HousekeepingAdvisorType::Event& aEvent)
					{
						JELLY_ALWAYS_ASSERT(aEvent.m_type == HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_WAL);
						JELLY_ALWAYS_ASSERT(aEvent.m_lowPrioWAL);
						JELLY_ALWAYS_ASSERT(aEvent.m_concurrentWALIndex == 1);
						JELLY_UNUSED(aEvent);
					}
				});

				JELLY_ALWAYS_ASSERT(blobNode.FlushPendingLowPrioWAL(1) == 1);
				JELLY_ALWAYS_ASSERT(blobNode.GetPendingLowPrioWALRequestCount(1) == 0);

				blobNode.Stop();
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
				BlobNodeType::Request req;
				req.SetKey(1);
				req.SetSeq(1);
				req.SetBlob(new UInt32Blob(123));
				blobNode.Set(&req);
				JELLY_ALWAYS_ASSERT(blobNode.ProcessRequests() == 1);

				// Since we now have a pending request, advisor should suggest a "flush pending WAL"
				_UpdateHousekeepingAdvisor(housekeepingAdvisor, 
				{
					[](const HousekeepingAdvisorType::Event& aEvent) { JELLY_ALWAYS_ASSERT(aEvent.m_type == HousekeepingAdvisorType::Event::TYPE_FLUSH_PENDING_WAL); JELLY_UNUSED(aEvent); }
				});

				blobNode.Stop();

				_TestLowPrioWALs();
			}

		}

	}

}