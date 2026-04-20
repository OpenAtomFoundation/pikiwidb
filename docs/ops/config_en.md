```
# Pika port
port : 9221

# Pika is multi-threaded; this parameter configures the number of Pika worker threads.
# It is not recommended to set this value higher than the number of CPU cores on the server.
thread-num : 1

# Size of the thread pool for processing user command requests
thread-pool-size : 8

# Number of threads on the slave for executing commands received from the master during master-slave sync
sync-thread-num : 6

# Task queue size for sync processing threads; not recommended to modify
sync-buffer-size : 10

# Pika log directory, used to store INFO, WARNING, ERROR logs as well as binlog (write2file) files for sync
log-path : ./log/

# Pika data directory
db-path : ./db/

# Size of each RocksDB memtable. Larger values improve write performance but cause greater I/O load when flushing.
# Configure appropriately based on your use case.
# Reference: https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide
write-buffer-size : 268435456

# Connection timeout in seconds. Pika starts counting down when a connection has no requests (enters sleep state).
# When the countdown reaches 0, Pika forcibly disconnects the connection.
# Properly configuring this parameter can prevent connection exhaustion. Default value is 60.
timeout : 60

# Admin password. Defaults to empty. If this is the same as userpass (including both being empty),
# the userpass parameter is automatically invalidated, and all users are treated as administrators
# without being restricted by the userblacklist parameter.
requirepass : password

# Sync authentication password. Used by the slave to authenticate when connecting to the master for sync.
# This must match the master's requirepass parameter.
masterauth :

# User password. Defaults to empty. If this is the same as requirepass (including both being empty),
# this parameter is automatically invalidated, and all users are treated as administrators.
userpass : userpass
 
# Command blacklist. Restricts users logged in with userpass from executing the listed commands.
# Commands are separated by commas. Default is empty.
# It is recommended to add high-risk commands here.
userblacklist : FLUSHALL, SHUTDOWN, KEYS, CONFIG

# Mode: classic or sharding. [classic | sharding]
# Classic mode supports multiple DBs.
instance-mode : classic

# In classic mode, specifies the number of DBs. Usage is the same as Redis.
databases : 1

# Number of slots when used with Codis
default-slot-num : 1024

# Defines the number of slave replicas in a replica group. Currently supported values: [0, 1, 2, 3, 4].
# 0 means this feature is disabled.
replication-num : 0

# Defines how many slave replicas must send ACKs before returning to the client.
# Currently configurable range: [0, ...replication-num]. 0 means disabled.
consensus-level : 0

# Prefix for Pika dump files. Files generated after bgsave will be named with this prefix.
dump-prefix : backup-
 
# Daemon mode [yes | no]
daemonize : yes
 
# slotmigrate [yes | no] (pika 3.0.0 does not support this parameter temporarily)
#slotmigrate : no

# Pika dump directory. Files generated after bgsave will be stored here.
dump-path : /data1/pika9001/dump/

# Dump directory expiry in days. Default is 0 (never expires).
dump-expire: 0

# pidfile path — directory for the pid file
pidfile : /data1/pika9001/pid/9001.pid
 
# Maximum number of connections
maxclients : 20000
 
# RocksDB SST file size. SST files are leveled: smaller files mean faster speed and lower compaction cost
# but more files; larger files mean relatively slower speed, higher compaction cost, but fewer files.
# Default is 20MB.
target-file-size-base : 20971520

# Binlog (write2file) file retention period in days. Minimum is 1.
# Files older than 7 days are automatically purged.
expire-logs-days : 7
 
# Maximum number of binlog (write2file) files to retain. Minimum is 10.
# When count exceeds 200, automatic cleanup begins, keeping the most recent 200 files.
expire-logs-nums : 200
 
# Number of guaranteed root user connections. Even when maxclients is exhausted,
# this parameter ensures 2 local (127.0.0.1) connections are still available to log into Pika.
root-connection-num : 2
 
# Slow log threshold in microseconds. Pika's slow log is written to pika-ERROR.log.
# Pika does not have a Redis-style slow log extraction API.
slowlog-log-slower-than : 10000

# Whether the slave is in read-only mode (yes/no, 1/0)
# slave-read-only : 0

# Pika DB sync path configuration
db-sync-path : ./dbsync/

# Controls the transfer speed during full sync. Properly configuring this avoids saturating the network card.
# Range: 1~1024 (MB/s). Values below 0 or above 1024 are automatically set to 1024.
db-sync-speed : -1 (1024MB/s)

# Specify network interface
# network-interface : eth1

# Sync configuration for slave nodes. Format: ip:port (e.g., 192.168.1.2:6666).
# After startup, this instance will automatically send a sync request to the specified master.
# slaveof : master-ip:master-port

# Server ID required for dual-master or hub configuration. Ignore if not using dual-master or hub.
server-id : 1

# Dual-master configuration. Ignore the following if not using dual-master.
double-master-ip :    # Peer master IP
double-master-port :  # Peer master port
double-master-server-id : # Peer master server ID
 
# Automatic full compact. Triggers a full compact at a configured time each day.
# Especially suitable for scenarios with many multi-data-structure expiries, deletions, and key name reuse.
# Format: "start_hour-end_hour/min_free_disk_percent". Example: to run compact daily from 3–4 AM
# only when disk free space is at least 30%, configure as: 03-04/30. Default is empty.
compact-cron : 

# Automatic full compact with interval. Unlike compact-cron (which runs once in a daily time window),
# compact-interval runs periodically at the specified interval (hours).
# Example: to run compact every 4 hours only when disk free space is at least 30%: 4/30. Default is empty.
compact-interval :

# Slave instance priority, used with Sentinel for failover only. Lower priority slaves are elected as master first.
# Default is 0 (does not participate in election).
slave-priority : 

# Used for cross-version sync between different Pika binlog formats. Can be set to [new | old].
# new: this instance can only be a slave of pika 3.0.0 and above; incompatible with pika 2.3.3~2.3.5.
# old: this instance can only be a slave of pika 2.3.3~2.3.5; incompatible with pika 3.0.0 and above.
# Default is new. Can be dynamically changed via config set when no sync relationship is configured.
# To change it after sync is configured, first run slaveof no one to disable sync, then use config set.
identify-binlog-type : new

# Sliding window size for master-slave sync flow control.
# In high-latency master-slave scenarios, increasing this parameter can improve sync performance.
# Default: 9000. Maximum: 90000.
sync-window-size : 9000

# Maximum buffer size for processing client connection requests. Default is 268435456 (256MB).
# Range: [64MB, 1GB]. Note: master and slave should have matching configurations.
# If a single command exceeds this buffer size, the server automatically closes the client connection.
max-conn-rbuf-size : 268435456

###################
#Critical Settings#
###################
# write2file file size. Default is 100MB. Cannot be modified after startup. Limited to [1K, 2G].
binlog-file-size : 104857600

# Compression algorithm [snappy, zlib, lz4, zstd]. Default is snappy. Cannot be modified after startup.
# Official binaries provide default static snappy linking. For other compression algorithms,
# download the corresponding static library and compile manually.
compression : snappy

# Number of background flush threads. Default is 1. Range: [1, 4].
max-background-flushes : 1

# Number of background compaction threads. Default is 1. Range: [1, 4].
max-background-compactions : 1

# Number of background threads. Default is 1. Range: [1, 4].
max-background-jobs : 1

# Maximum number of open files DB can use. Default is 5000.
max-cache-files : 5000

# Upper limit of total memtable size used by all RocksDB instances owned by this Pika instance.
# If actual usage exceeds this value, the next write will trigger a flush.
# Reference: https://github.com/facebook/rocksdb/wiki/Setup-Options-and-Basic-Tuning
max-write-buffer-size : 10737418240

# Limits the maximum size of command response data, to handle commands like keys * that
# could return huge amounts of data and exhaust memory.
max-client-response-size : 1073741824

# Level multiplier in the Pika engine, controlling the total capacity ratio between each level
# and the level above it. Default is 10x; can be adjusted to 5x.
max-bytes-for-level-multiplier : 10

# Count operations on keys; perform small-scale compaction on frequently-operated keys.
# max-cache-statistic-keys is the number of monitored keys. Set to 0 to disable this feature.
max-cache-statistic-keys : 0

# If small-scale compaction is enabled, perform compaction on a key when operation count
# exceeds small-compaction-threshold.
small-compaction-threshold : 5000
```

# Data Directory Description

#### db directory
Used to store all of Pika's data files. Contains 5 subdirectories (one for each of the 5 major data types): kv, set, zset, hash, list. Starting from Pika 3.0.0, these data structure directories are: hashes, lists, sets, strings, zsets.

#### log directory
Used to store all log files, including: general logs, warning logs, error logs, sync logs (binlog), and the sync log index file (manifest).

#### dump directory
Used to store files produced by snapshot-based backups.

#### pid directory
Used to store Pika's pid file.

#### dbsync directory
Used to store files needed during a full master-slave sync.
