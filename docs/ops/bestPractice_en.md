### Based on 360's internal Pika usage experience and community user feedback, we have compiled the following documentation and will continue to update it.
> To avoid losing this document in the future, you can click the star button in the upper right to follow.

**Pika Best Practice #0:**

Actively including your version number when asking questions in the group can greatly speed up issue resolution (QQ group: 294254078).

**Pika Best Practice #1:**

We recommend using the latest version of 3.0. If you are unwilling to use 3.x, please use 2.3.6. Otherwise, many issues you encounter will already be in our bug fix list. (Version 2.0 is no longer maintained.)

**Pika Best Practice #2:**

The recommended thread count for Pika is equal to the total CPU thread count. For multi-instance deployments on a single machine, the thread count per Pika instance can be reduced accordingly, but is not recommended to be lower than 1/2 of the total CPU thread count.

**Pika Best Practice #3:**

Pika's performance is closely tied to I/O performance. We do not recommend deploying latency-sensitive Pika instances on spinning disks. Also, to avoid strange issues, master and slave servers should have as similar hardware performance as possible.

**Pika Best Practice #4:**

When using Pika with multi-data-structures, try to keep the number of fields per key manageable. It is recommended to keep the field count per key under 10,000. Very large keys can be split into multiple smaller keys to avoid the many potential performance risks of oversized keys.

**Pika Best Practice #5:**

The `root-connection-num` parameter is very useful — it specifies the "number of connections allowed to log into Pika via 127.0.0.1." It is independent of `maxclients`. Exhausting `maxclients` does not affect `root-connection-num`, so when `maxclients` is exhausted in an abnormal scenario, the administrator can still log into the Pika server locally and connect via 127.0.0.1 to handle the issue, avoiding the awkward situation of being unable to log in.

**Pika Best Practice #6:**

The `client kill` command has been enhanced. To kill all current connections to Pika at once, simply execute `client kill all`. Don't worry — the sync connections will not be affected.

**Pika Best Practice #7:**

Appropriately configure the `timeout` parameter. Through this parameter, Pika will proactively disconnect connections that have been inactive for longer than the `timeout` value, preventing connection exhaustion. Since connections also consume memory, properly configuring `timeout` can also reduce Pika's memory usage to some extent.

**Pika Best Practice #8:**

Pika's memory usage is mainly concentrated in SST file cache and connection-allocated memory. Connection memory is usually much larger than SST cache. Pika currently supports dynamic adjustment and reclamation of connection-allocated memory, so the total connection memory can be roughly estimated. If Pika's memory usage far exceeds estimates or exceeds 10 GB, there may be a memory leak. Try executing `client kill all` and `tcmalloc free` (tcmalloc has been removed) to force reclaim connection memory. If ineffective, upgrade to the latest version.

**Pika Best Practice #9:**

It is strongly not recommended to run Pika standalone. The minimum cluster state should be one master and one slave. There are many failover options for master-slave clusters: LVS, VIP floating, configuration management middleware, etc.

**Pika Best Practice #10:**

It is recommended to use master-slave clusters rather than dual-master mode. In practice, dual-master mode has higher requirements for usage discipline and network environment. Poor discipline or bad network conditions can cause issues in dual-master mode. Data recovery in dual-master mode is more complex than in master-slave clusters.

**Pika Best Practice #11:**

If your Pika runs standalone (not in master-slave or master-master clusters) and is deployed on reliable storage, you can consider improving write performance by disabling binlog (setting `write-binlog` to `no`). However, we do not recommend running standalone — at least one slave should exist for failover. So disabling binlog is not recommended for non-standalone setups.

**Pika Best Practice #12:**

Pika's data directory contains a large number of SST files that grow with the amount of data. You need to configure a higher `open_file_limit` to avoid running out of file descriptors. If you want to reduce the number of file descriptors used by Pika, you can increase the size of individual SST files to reduce the total SST count, using the `target-file-size-base` parameter.

**Pika Best Practice #13:**

Do not modify the `write2file` files and `manifest` files in the log directory. They are important files related to replication. `write2file` acts as `binlog`, while `manifest` ensures that after an instance restart, binlog writing can resume, and a slave instance can resume incremental sync after reconnection.

**Pika Best Practice #14:**

Pika's full sync is done via rsync. We provide the rsync transfer rate limiting parameter `db-sync-speed` (in MB). We recommend this parameter should not exceed 75 in a gigabit network and not exceed 500 in a 10-gigabit network, to avoid Pika consuming all network bandwidth during full sync and impacting other services on the same server.

**Pika Best Practice #15:**

Executing `keys *` in Pika will not block Pika (Pika is multi-threaded), but in scenarios with an enormous number of keys, it may temporarily consume a large amount of memory (to store the result of `keys *` for that connection, which is released after the operation completes). Therefore, use `keys *` with extreme caution.

**Pika Best Practice #16:**

If Pika has data but `info keyspace` shows all zeros, this is because Pika does not maintain real-time key count statistics like Redis. Key statistics in Pika must be manually triggered by executing `info keyspace 1`. Note that `info keyspace` without the `1` parameter will only show the last scan result without triggering a new scan. Key counting takes time (it is asynchronous). You can check progress via `is_scaning_keyspace` in `info stats` — `yes` means a scan is in progress, `no` means no scan is running or the last scan has ended. `info keyspace` data is not updated until the scan completes. The data is stored in memory and will be reset to zero on restart.

**Pika Best Practice #17:**

Do not trigger key scanning (`info keyspace 1`) or run `keys *` while Pika is performing a full `compact`, as this will temporarily inflate the data size until the key scan or `keys *` completes.

**Pika Best Practice #18:**

Configuring `compact-cron` for instances with many expired entries or frequent multi-data-structure element operations can prevent invalid but not-yet-fully-cleaned data from impacting performance. Or upgrade to version 3.0 and enable the new key-level `auto_compact` feature.
If you encounter the following situations, your instance may have performance risks from invalid data not being cleaned up in time:
1. Abnormally large data size (more than 10% above estimate). You can run `compact` and check if the data size returns to normal after completion.
1. Request latency suddenly increases abnormally. You can run `compact` and check if request latency returns to normal after completion.

**Pika Best Practice #19:**

In Pika 3.0, we provide statistics for expired keys (triggerable via `info keyspace 1` and viewable via `info keyspace`). The `invaild_keys` value in the statistics represents "keys that have been deleted/expired but not yet physically deleted." It is recommended to monitor this value and use `compact` to clean up when the count of invalid keys is high. This keeps physically uncleared invalid data under control, ensuring stable Pika performance. If data in Pika expires on a regular schedule (e.g., each key expires in 7 days), it is recommended to configure `compact-cron` for a scheduled daily full automatic `compact`. Since `compact` consumes some I/O resources, if disk I/O pressure is high, configure it to run during off-peak hours (e.g., late at night).

**Pika Best Practice #20:**

`write2file` acts as `binlog`. Its retention period/count should be adjusted based on actual write volumes. It is recommended to retain `write2file` for no less than 48 hours. Sufficient `write2file` makes many situations easier to handle, such as: expanding slave nodes in a large data cluster, maintenance shutdown of slave servers, slave migration, etc. Without sufficient `write2file`, the master may have purged the logs, forcing a full re-sync.

**Pika Best Practice #21:**

When the master has extremely heavy write volume (roughly over 50k write QPS on a regular SSD), slaves may experience sync lag. You can adjust the slave's `sync-thread-num` parameter to improve slave sync performance. This parameter controls the number of sync threads on the slave. Each thread is responsible for the corresponding keys via hashing. The more unique keys in the master's write operations, the better the effect. If heavy writes are concentrated on just a few keys, this parameter may not achieve the expected effect.

**Pika Best Practice #22:**

Pika's backups are generated as snapshots via hard links stored in the dump directory with a date suffix. Only one backup can be generated per day; new backups overwrite previous ones. During snapshot generation, Pika briefly blocks writes to ensure data consistency. The block time is related to actual data volume — tests show even a 500 GB Pika needs only 50 ms to generate a snapshot. During the write block, connections are not interrupted and requests don't fail, but clients may notice "slightly higher latency at that moment." Since Pika's snapshot is a hard link to SST files in the DB directory, the backup initially uses no extra disk space. Once SST files in the Pika DB directory are merged or deleted, the hard links will reflect real sizes and start consuming disk space. Adjust backup retention days based on actual disk space to avoid disk exhaustion.

**Pika Best Practice #23:**

If write volume is very high and disk performance cannot keep up with RocksDB memtable flush demands, RocksDB may enter write protection mode (all writes will be blocked). For this issue, we recommend switching to higher-performance storage, or reducing write frequency (e.g., spreading 2 hours of concentrated writes over 4 hours). You can also moderately increase `write-buffer-size` to increase total memtable capacity, reducing the likelihood of all memtables filling up. However, tests show this doesn't fully resolve the issue, as "the memtable still needs to be flushed eventually!"

**Pika Best Practice #24:**

Pika compresses data, with the default compression algorithm being `snappy` (changeable to `zlib`). Every data store and read involves compression/decompression, which consumes some CPU. It is strongly recommended to use Pika like Redis: disable compression in Pika, and handle compression/decompression in the client. This not only reduces data size but also effectively reduces Pika's CPU pressure. If storage space is not a concern but you don't want to adjust the client, you can disable compression to reduce CPU pressure at the cost of higher disk usage. Note that toggling compression requires an instance restart but no data migration.

**Pika Best Practice #25:**

Read-write separation is important. In a typical master-slave cluster, write operations are single-point (master), so write throughput has a ceiling. Read operations can be distributed across multiple slaves. Therefore, read throughput scales with the number of slaves. For high-read scenarios, it is recommended to add read-write separation logic in the application layer and increase the number of slaves to provide read service together, greatly improving cluster stability and reducing read latency.

**Pika Best Practice #26:**

Full compact progressively merges and cleans data at each RocksDB level. During this process, many SST files are created and deleted. Therefore, during a full `compact`, the data size first increases then decreases, eventually settling at a stable value (after merging and cleaning invalid/duplicate data, only valid data remains). Before executing `compact`, it is recommended to ensure disk free space is at least 30% to avoid running out of disk space when new SST files are created. Also, Pika supports `compact` on specific data structures. For example, if you know a hash structure has very few invalid entries but a large data volume, and a set structure has a large amount of invalid data, the hash `compact` is unnecessary. You can use `compact set` to compact only the set structure.

**Pika Best Practice #27:**

Backups are produced as hard links to SST files in the DB directory. With backup files present, once a full compact is run (since all old SSTs in the Pika DB directory will be "cleaned" — progressively deleted and replaced with new SSTs), backup hard link file sizes will reflect their true size. In extreme cases, backup files may consume an extra copy of the disk space. Therefore, if your disk free space is not ample, it is best to delete backups before running a full `compact`.

**Pika Best Practice #28:**

Pika supports slow log functionality like Redis, accessible via the `slowlog` command. However, `slowlog` storage has an upper limit depending on your configuration. If configured too high, `slowlog` may consume too much memory. Pika allows recording slow logs in the `pika.ERROR` log file for tracing and analysis. This feature requires setting `slowlog-write-errorlog` to `yes`.

**Pika Best Practice #29:**

Pika does not provide Redis's `rename-command` functionality, because renaming certain commands can cause tools and middleware to malfunction (e.g., renaming `config` breaks Sentinel). Therefore, Pika added `userpass` and `userblacklist` to address this. `userpass` corresponds to `requirepass` — users logged in with `userpass` are restricted by `userblacklist` and cannot execute commands listed there. `requirepass` is unrestricted. Think of users logging in with `requirepass` as "super users" and those with `userpass` as "regular users." It is strongly recommended to provide `userpass` to application code and add high-risk commands to `userblacklist`, such as `slaveof`, `config`, `shutdown`, `bgsave`, `dumpoff`, `client`, `keys`, etc., to prevent accidents.

**Pika Best Practice #30:**

In Pika 3.0.7, the network library was refactored. Previously, both network communication and data query/insert operations ran in the threads configured by `thread-num`. After the refactor, network communication still runs in `thread-num` threads, while data write and delete operations run in the thread pool controlled by `thread-pool-size`. Users can adjust these two parameters based on their scenarios. For heavy operations, increasing `thread-pool-size` helps. Generally, we recommend `thread-pool-size = 2 * thread-num`.

**Pika Best Practice #31:**

Starting from Pika 3.0.5, a more granular compact strategy is provided. It monitors key operations and performs a targeted compact on a key when a configured threshold is reached. This feature only applies to hash, set, zset, and list (the four data structures with fields). Related parameters:
* `max-cache-statistic-keys`: Sets the number of monitored keys, e.g., 10000 (monitor 10,000 keys).
* `small-compaction-threshold`: Sets the operation threshold (how many fields in that key have been modified, or how many times they've been modified), e.g., 500.

This feature is especially suitable (but not limited to) for the following or similar scenarios, ensuring performance stability by timely cleaning up invalid data:

1. Large amounts of hash data with frequent modifications, additions, and deletions of fields within a key.
2. Large amounts of list structures used in a queue-like manner.
3. Frequent direct deletion of multi-data-structure keys followed by reuse.

**Pika Best Practice #32:**

When business load is high and I/O utilization is high, avoid running `compact`. Under insufficient I/O resources, the I/O from `compact` may compete with business requests, degrading overall instance performance. If you accidentally run it, simply restart Pika — data will not be corrupted or lost. The SST files left over from an incomplete `compact` will be automatically and safely cleaned up by RocksDB.

**Pika Best Practice #33:**

If you find a slave repeatedly doing full syncs when establishing master-slave replication, it is likely because the master's write volume is too high, and the binlog position at the time of the dump has been purged by the time full sync completes, causing the slave to re-trigger full sync after replacing the DB and finding the position again — a cycle. In this case, you can dynamically increase `expire-logs-nums` on the master to retain more binlog files. After the master-slave relationship is successfully established, revert the value.

---
_Continuously updated_
