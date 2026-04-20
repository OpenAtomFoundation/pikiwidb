### 1 Compilation and Installation

#### Q1: Which operating systems are supported?
A1: Currently only Linux environments are supported, including CentOS and Ubuntu. **Windows and Mac are not supported.**

#### Q2: How to compile and install?
A2: Refer to the [compilation and installation wiki](install.md).

#### Q3: Ubuntu compilation occasionally reports the error `isnan isinf was not declared`?
A3: Some older versions of pika have poor Ubuntu compatibility and this can occur in certain situations. You can first modify the code, replacing `isnan` and `isinf` with `std::isnan` and `std::isinf`, and include the header file `cmath`. We will add compatibility for this in a new version.
```
#include <cmath>
```

### 2 Design and Implementation
#### Q1: Why open so many threads? For example, for purge, wouldn't a timer task be enough? Does the programming framework not support timers?
A1: Pika has some time-consuming tasks, such as deleting binlogs, scanning keys, backing up data, syncing data files, etc. To avoid impacting normal user requests, these tasks are executed in the background. Tasks that can run in parallel are placed in different threads to maximize background task execution speed. The "programming framework" you mentioned is pink (the network library, code located at src/net). Pink does support timers — every WorkerThread, as long as the user defines a cronhandle and a frequency, will periodically execute the defined content. However, at that time the worker is exclusively occupied and cannot respond to user requests, so time-consuming tasks are better handled in separate threads. Redis's bio (background I/O) serves the same purpose.

#### Q2: Wouldn't it be better to have the sender handle heartbeats? Or is it necessary to have so many threads in the sender?
A2: There are mainly two reasons. First, to improve sync speed — the sender only sends, the receiver only receives, and heartbeats are handled by a separate thread. If the sender handled heartbeats, it would complicate the sender and receiver logic for just one heartbeat per second. Second, in earlier attempts to combine them for connection-level liveness detection, under heavy write pressure, heartbeat packet processing was delayed, causing false detection of master timeouts and unnecessary reconnections by slaves.

#### Q3: In nemo's storage of hash actual keys, is the first byte a header? A type marker? Is it indicating it's a hash type?
A3: It is indeed a header, but not to mark it as a hash type. Nemo already separates string, hash, list, zset, and set into 5 separate DBs, so they don't interfere with each other. The header exists because a hash has one meta key and a bunch of field keys. The meta key's value records basic information about the hash (such as the hash size). The header is used to distinguish meta keys from field keys.

#### Q4: What is `curr_seq` in the list data structure?
A4: The list implementation is fully based on KV. It uses sequences to implement list-like prev and next pointers. `curr_seq` is in the meta information — it represents the current sequence used so far. New nodes use sequences starting from this and increment upward.

#### Q5: Does the binlog store transformed put/delete operations, or the original Redis commands?
A5: It stores Redis commands.

#### Q6: Is the rsync daemon mode using Linux's rsync command?
A6: Yes. In early stages, Pika directly called the rsync command to quickly implement full sync data file sending and receiving, which also handles file resume and checksum verification.

#### Q7: Is the dump DB file a built-in RocksDB feature? How does it work?
A7: RocksDB provides a snapshot backup feature for the current DB. We use this feature: when dumping, Pika first blocks user writes, records the current binlog offset, and calls RocksDB's interface to get the current DB metadata. At this point, user writes can resume. Then a background copy of the snapshot data is performed based on this metadata. The write-blocking time is very short.

#### Q8: If we write to binlog first and then execute, what if the server crashes before the command executes but after writing to binlog?
A8: The master writes to DB first, then writes to binlog. Previously, using a single worker on the slave for sync could cause large sync gaps when the master had heavy write pressure. Later, we adjusted the structure so the slave uses multiple workers to write, improving write speed. However, this creates a problem: to ensure master-slave binlog order consistency, only one thread can write to binlog — the receiver. So on the slave side, binlog is written first, then DB. This means the slave may lose data if it crashes after writing binlog. However, Redis also loses data if the master crashes after writing to DB, and Redis uses full sync to resolve this. Pika does the same — by default uses incremental sync to continue; if the business is very data-sensitive, the slave can be forced to do a full sync on restart.

#### Q9: BinlogBGWorker threads still need to execute in binlog order; how much concurrency improvement does this actually achieve?
A9: The earlier master-slave sync gap was caused by the master using multiple workers for writing while the slave only had one worker. The new approach improves the slave's DB write speed, but protocol parsing is still single-threaded, which is still a bottleneck. However, this optimization improved master-slave sync performance by approximately 3~5x. If there are very few unique keys, the improvement may be less obvious, because on the slave side, workers are sharded by the key's hash value.

#### Q10: Instant-delete: Does every put operation require querying the latest version of the key? Does every write always involve an extra read?
A10: Pika's multi-data-structure implementation mainly uses "meta key + regular keys." For multi-data-structure read/write, RocksDB is accessed at least 2 times. The version information you mentioned is stored in the meta_key and is read out along with other meta information — no extra read/write is added because of the version number.

#### Q11: Why does Pika use multi-threading instead of Redis's single-threaded structure?
A11: Because all Redis operations are memory operations, each Redis operation is theoretically very fast. Pika involves disk I/O, so multi-threading is necessary to fully utilize hardware resources.

#### Q12: Is data sharding done at the proxy layer? For collection operations like mget where data falls into different slots, is aggregation done at the proxy layer?
A12: Currently there is no data sharding. You can think of it as similar to standalone Redis, supporting a master-slave architecture. A single Pika instance's storage limit is the disk size.

#### Q13: What clients does Pika support? Is pipelining supported?
A13: Pika supports all Redis clients. Since pipelining is a client-side feature, we support it.

#### Q14: Why not consider Redis cluster sharding?
A14: When we started building Pika, Redis cluster sharding was not yet mature, and the use cases for Redis cluster and Pika are also different. We haven't widely adopted Redis cluster internally yet.

#### Q15: Why use LVS in front? Redis-like services are stateful — doesn't load balancing cause issues?
A15: We expose the LVS IP to users. LVS is placed in front of Redis for convenient master-slave failover, so users don't perceive the switch. Multiple Redis instances behind LVS all have master-slave structure.

#### Q16: Have you compared with SSDB and LevelDB? What are the advantages?
A16: Our internal teams have used SSDB, but most SSDB instances (except games) have been migrated to Pika. I think Pika's advantage lies in careful implementation details and better performance.

#### Q17: Why not choose LevelDB as the storage engine? And how is Pika different from similar solutions like SSDB?
A17: We compared LevelDB and RocksDB. The performance difference is small when data is small, but when data is large (e.g., 200 GB), RocksDB significantly outperforms LevelDB. However, RocksDB's code is not as elegant as LevelDB's.

#### Q18: Similar to standalone Redis, isn't single-machine performance a bottleneck? Large client connections, command processing, network bandwidth, etc.?
A18: Yes. Our internal Pika architecture supports 1-master-multiple-slaves and multi-datacenter self-contained solutions. Currently, the largest production setup is 1 master with 14 slaves. DBAs can easily add slaves with `slaveof` and trigger full sync.

#### Q19: Pika's multi-threading outperforms Redis's full-memory model in `get` performance by 2x? And `set` is also faster — isn't there multi-threading lock overhead?
A19: The test used 18 Pika worker threads. For KV structures, lock overhead is minimal. For complex structures like hash and zset, even with 18 threads, performance is still lower than Redis due to metadata lock contention. But KV has nearly no lock overhead beyond RocksDB's internal thread-safety locks.

#### Q20: Is it entirely because of uneven distributed sharding that the distributed cluster was abandoned? Doesn't M-S architecture mean every node stores all data, consuming more resources?
A20: We did add multi-data-structure interfaces to bada with Redis protocol compatibility, but users' actual data volumes were smaller than expected — a single machine with 1 TB disk could handle it. Due to hash distribution imbalance, maintenance costs increased, so we implemented the M-S architecture instead.

The bada solution coexists with Pika; we recommend storage solutions based on specific use cases. No single storage solution solves all internal needs.

#### Q21: Besides being comparable to standalone Redis, have you considered distributed support like Redis sentinel or Codis for seamless migration?
A21: Pika currently does not use Redis sentinel; we use LVS for master-slave switching. We also haven't used Codis-like proxy solutions.

#### Q22: 1 master with 14 slaves — isn't master-slave sync very slow? Also, slaves are read-only, right? Data on slaves could be stale — how is data consistency handled?
A22: The 1-master-14-slave scenario is for write workloads done overnight in batches, with reads distributed across slaves. This data consistency trade-off is acceptable for those users.

#### Q23: With `expire-logs-nums` (at least 10) and binlog expiration time set, why does the master still have large numbers of write2file files?
A23: Pika periodically checks binlog files. If the count exceeds `expire-logs-nums` or they are expired, and all slaves have already synced those binlog files, they will be deleted. Make sure `expire-logs-nums` and expiration time are correctly configured. Use the `info` command to check if any slave has large sync delays preventing binlog deletion.

##### If you have other questions, please describe them directly on the GitHub issue page and we'll respond as soon as possible. Join QQ group 294254078 where we post updates periodically.
