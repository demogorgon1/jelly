# Jelly

## Documentation

* [Lock and Blob Nodes](https://github.com/demogorgon1/jelly/wiki/Nodes)
* [Class Reference](https://demogorgon1.github.io/jelly/annotated.html)

## Examples

If you read through these examples in order, you should get a pretty good idea of how _jelly_ works. 

* [Example 1](https://demogorgon1.github.io/jelly/Example1_BlobNode_8cpp-example.html) - Basic example of how to save and load blobs.
* [Example 2](https://demogorgon1.github.io/jelly/Example2_LockNode_8cpp-example.html) - Basic example of how to lock and unlock a key.
* [Example 3](https://demogorgon1.github.io/jelly/Example3_Housekeeping_8cpp-example.html) - Illustrates how to do housekeeping operations.
* [Example 4](https://demogorgon1.github.io/jelly/Example4_HousekeepingAdvisor_8cpp-example.html) - Using the housekeeping advisor to decide when housekeeping tasks should be done.

## Configuration 

Option|Type|Restart Required|Default|Description
-|-|-|-|-
compression_method|string|Yes|zstd|Specifies how data should be compressed: ```none``` or ```zstd```. Lock node WALs and stores are stream compressed while blob nodes apply individual item compression to allow random access.
compression_level|uint32|Yes|1|The compression level for the selected compression method. Usually in the range of 1 to 9, but depends on the compression method.
wal_size_limit|size|No|64MB|If a WAL file reaches this size it will be closed and a new one created.
wal_concurrency|uint32|Yes|1|Number of WALs to keep open at the same time. These WALs can be flushed in parallel.
backup_path|string|No|backups|Path to where backups should be created. This path must point to a directory that is on the same disk volume as the host root due to the creation of hard links.
backup_compaction|bool|No|true|Perform compaction as part of the backup process. If enabled backed up stores will be turned into a single store.
backup_incremental|bool|No|true|Backups only include new stores since last backup.
max_resident_blob_size|size|No|1GB|Total size of blobs to keep resident (cached). If blobs exceed this threshold, the oldest ones will be removed from the cache. If they're later requested they'll be read from a store on disk.
max_resident_blob_count|size|No|1G|Maximum number of blobs to keep resident (cached). If blobs exceed this threshold, the oldest ones will be removed from the cache. If they're later requested they'll be read from a store on disk.
min_wal_flush_interval|interval|No|500ms|HousekeepingAdvisor: Duration between WAL flushes will never be shorter than this.
max_cleanup_wal_interval|interval|No|2m|HousekeepingAdvisor: Maximum time between WAL cleanups. Usually they'll be triggered after each pending store flush as well.
min_compaction_interval|interval|No|5s|HousekeepingAdvisor: Duration between compactions will never be shorter than this.
pending_store_item_limit|uint32|No|30000|HousekeepingAdvisor: If number of unique pending items exceed this threshold a pending store flush will be triggered.
pending_store_wal_item_limit|uint32|No|300000|HousekeepingAdvisor: If number of items written to pending WALs exceed this a pending store flush will be triggered.
pending_store_wal_size_limit|size|No|128MB|HousekeepingAdvisor: If total size of all WALs exceed this a pending store flush will be triggered.
compaction_size_memory|size|Yes|10|HousekeepingAdvisor: Number of total store size samples to remember back. This is used by the compaction strategy.
compaction_size_trend_memory|size|Yes|10|HousekeepingAdvisor: Derivative of the store size (velocity) samples. This is used by the compaction strategy.
compaction_strategy|string|Yes|stcs|HousekeepingAdvisor: Compaction strategy to use. Currently ```stcs``` (Size-Tiered Compaction Strategy) is the only option.
compaction_strategy_update_interval|interval|No|10s|HousekeepingAdvisor: How often to update the compaction strategy.
stcs_min_bucket_size|size|No|4|HousekeepingAdvisor: Minimum bucket size for STCS compaction.