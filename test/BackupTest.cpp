#include <jelly/API.h>

#include "UInt32Blob.h"

namespace jelly
{

	namespace Test
	{

		namespace
		{

			void
			_Backup(
				bool		aCompaction)
			{
				std::filesystem::remove_all("backups");

				DefaultConfigSource config;
				config.Set(Config::ID_BACKUP_COMPACTION, aCompaction ? "true" : "false");
				config.Set(Config::ID_BACKUP_INCREMENTAL, "true");

				DefaultHost host(".", "backuptest", &config);
				host.DeleteAllFiles(UINT32_MAX);

				typedef BlobNode<UIntKey<uint32_t>> BlobNodeType;
				BlobNodeType blobNode(&host, 0);

				// Make some stores we can back up
				for(uint32_t i = 0; i < 10; i++)
				{
					BlobNodeType::Request req;
					req.SetKey(1000 + i);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(i));
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ASSERT(blobNode.FlushPendingWAL() == 1);
					JELLY_ASSERT(blobNode.FlushPendingStore() == 1);
				}

				// Initial backup
				{
					std::unique_ptr<BlobNodeType::BackupType> backup(blobNode.StartBackup("foo"));
					JELLY_ASSERT(backup);
					JELLY_ASSERT(!backup->IsIncremental());

					if(aCompaction)
						JELLY_ASSERT(backup->GetIncludedStoreIdCount() == 1);
					else
						JELLY_ASSERT(backup->GetIncludedStoreIdCount() == 10);

					backup->Perform();
					blobNode.FinalizeBackup(backup.get());
				}

				// Make a bunch of more stores
				for (uint32_t i = 100; i < 110; i++)
				{
					BlobNodeType::Request req;
					req.SetKey(1000 + i);
					req.SetSeq(1);
					req.SetBlob(new UInt32Blob(i));
					blobNode.Set(&req);
					JELLY_ASSERT(blobNode.ProcessRequests() == 1);
					JELLY_ASSERT(blobNode.FlushPendingWAL() == 1);
					JELLY_ASSERT(blobNode.FlushPendingStore() == 1);
				}

				// Do another backup, this time it should be incremental
				{
					std::unique_ptr<BlobNodeType::BackupType> backup(blobNode.StartBackup("bar"));
					JELLY_ASSERT(backup);
					JELLY_ASSERT(backup->IsIncremental());
					JELLY_ASSERT(backup->GetIncrementalDependency() == "foo");

					if (aCompaction)
						JELLY_ASSERT(backup->GetIncludedStoreIdCount() == 1);
					else
						JELLY_ASSERT(backup->GetIncludedStoreIdCount() == 10);

					backup->Perform();
					blobNode.FinalizeBackup(backup.get());
				}
			}

		}

		namespace BackupTest
		{

			void		
			Run()
			{
				_Backup(true); // With compaction
				_Backup(false); // Without compaction
			}

		}

	}

}
