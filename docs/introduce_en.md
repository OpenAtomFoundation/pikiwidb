## What is Pika

Pika is a Redis-compatible storage system jointly developed by the DBA and Infrastructure teams. It fully supports the Redis protocol, allowing users to migrate services to Pika without modifying any code. Pika is a persistent, large-capacity Redis storage service, compatible with the vast majority of string, hash, list, zset, and set interfaces ([compatibility details](ops/API.md)). It resolves the capacity bottleneck of Redis caused by storing enormous amounts of data exceeding available memory. Like Redis, it supports master-slave replication via the `slaveof` command, with both full sync and incremental sync. The DBA team also provides migration tools so users won't feel the migration process — it is seamless.

## Comparison with Redis
The biggest difference between Pika and Redis is that Pika is persistent storage, with data stored on disk, while Redis is an in-memory storage. This difference brings Pika both advantages and disadvantages compared to Redis.

### Advantages:
1. Large capacity: Pika has no memory limit like Redis; the maximum storage is equal to the disk capacity.
2. Fast DB loading: Pika writes data to disk, so even if a node crashes, no rdb or oplog is needed. Pika restarts without loading all data into memory, requiring no data replay.
3. Fast backup: Pika backup speed is roughly equivalent to `cp` speed (with an additional snapshot recovery process that takes some time). This makes backing up databases of hundreds of GB quick, and faster backups better resolve the full-sync problem in master-slave scenarios.

### Disadvantages:
Since Pika stores data both in memory and on disk, its performance is somewhat lower than Redis. However, we generally use SSD drives to store data, trying to keep up with Redis performance as much as possible.

## Use Cases
From the comparison above, if your business data is large and Redis can barely support it (e.g., greater than 50 GB), or your data is critical and must not be lost on power failure, Pika can solve your problems.
In practice, Pika's performance is approximately 50% of Redis.

## Features of Pika
1. Large capacity, supporting hundreds of GB of data storage
2. Redis-compatible, allowing smooth migration from Redis to Pika without code changes
3. Supports master-slave replication (slaveof)
4. Comprehensive operations commands

## Current Deployment Status
Currently, Pika is deployed and running more than 20 giant clusters in production.
Rough statistics: the current total daily requests exceed 10 billion, and the total amount of data served is approximately 3 TB.

## Performance Comparison with Redis
### Configuration
- CPU: 24 Cores, Intel® Xeon® CPU E5-2630 v2 @ 2.60GHz
- MEM: 165157944 kB
- OS: CentOS release 6.2 (Final)
- NETWORK CARD: Intel Corporation I350 Gigabit Network Connection

### Test Process
First, 150 GB of data was written to Pika — 50 Hash keys were written with 10 million fields each.
Redis was written with 5 GB of data.
Pika: 18 threads
Redis: single thread
 ![](images/benchmarkVsRedis01.jpeg)

### Conclusion
Pika's single-thread performance is certainly inferior to Redis. Pika has a multi-threaded structure, so with a higher thread count, the performance of certain data structures can surpass Redis.


## Pika Performance Overview in Specific Scenarios
### Pika vs SSDB ([Detail](pikaVsSSDB.md))

<img src="images/benchmarkVsSSDB01.png" height = "400" width = "480" alt="1">

<img src="images/benchmarkVsSSDB02.png" height = "400" width = "480" alt="10">

## Pika vs Redis
<img src="images/benchmarkVsRedis02.png" height = "400" width = "600" alt="2">

## How to Migrate from Redis to Pika

### What Developers Need to Do
Developers don't need to do anything — no code changes, no driver replacement (Pika uses the native Redis driver). Just watch the DBA do the work.

### What DBAs Need to Do
1. DBA migrates Redis data to Pika
1. DBA synchronizes Redis data to Pika in real time, ensuring data consistency between Redis and Pika
1. DBA switches the LVS backend IP, replacing Redis with Pika
