## Pika 4.0 to Redis Migration Tool

### Applicable Versions:
Pika 4.0, standalone mode, single DB only.

### Functionality
Online migration of data from Pika to Pika or Redis (supports full and incremental synchronization).

### Development Background
The previously official `pika_to_redis` tool provided by the Pika project only supported offline migration of data from Pika's DB to Pika or Redis, and did not support incremental synchronization. This tool is essentially a special Pika instance: after becoming a replica, it internally forwards data received from the primary to Redis while also supporting incremental synchronization, enabling hot (live) migration.

### Hot Migration Principle
1. pika-port sends a `dbsync` request to obtain the current full DB data of the primary, along with the binlog position corresponding to the current DB data.
2. After obtaining the full DB data from the primary, it scans the DB and forwards the data to Redis.
3. Using the previously obtained binlog position, it performs incremental synchronization with the primary. During incremental synchronization, the binlog received from the primary is reassembled into Redis commands and forwarded to Redis.

### New Configuration Items
```cpp
###################
## Migrate Settings
###################

target-redis-host : 127.0.0.1
target-redis-port : 6379
target-redis-user :
target-redis-pwd  :

sync-batch-num    : 100
redis-sender-num  : 10
```

### Steps
1. Since writing all the full data to Redis may take a long time (during which the primary may have already purged the original binlog positions), first run `config set expire-logs-nums 10000` on the primary to keep 10,000 binlog files. (Binlog files consume disk space; adjust the retention count based on your actual situation.) This ensures that the corresponding binlog files still exist when this tool requests incremental synchronization later.
2. Modify the tool's configuration file for `target-redis-host`, `target-redis-port`, `target-redis-pwd`, `sync-batch-num`, and `redis-sender-num`. (`sync-batch-num` controls how many data entries are packed and sent together to Redis at once to improve forwarding efficiency after receiving full data from the primary. Additionally, `redis-sender-num` threads can be specified internally to forward commands; commands are distributed to different threads based on the hash value of the key, so there is no need to worry about data disorder caused by multi-threaded sending.)
3. Start the tool with `pika -c pika.conf` and check the logs for any errors.
4. Execute `slaveof ip port force` on the tool to request synchronization from the primary, and observe whether there are any errors.
5. After confirming that the primary-replica relationship has been established successfully (at which point pika-port is also forwarding data to the target Redis), check the replication lag by running `info Replication` on the primary. (You can also write a special key to the primary and check whether it can be immediately retrieved from Redis to determine whether data synchronization is essentially complete.)

### Notes
1. Pika supports different data structures with the same key name, but Redis does not. In scenarios where the same key name exists across different data structures, the first data structure migrated to Redis takes precedence, and other data structures with the same key will be lost.
2. This tool only supports hot migration in standalone mode with a single-DB version of Pika. If cluster mode or multi-DB scenarios are used, the tool will report an error and exit.
3. To avoid dirty data being written to Redis due to the tool triggering multiple full synchronizations caused by the primary's binlog being purged, the tool has built-in protection: it will report an error and exit when a second full synchronization is triggered.
