#### config [get | set | rewrite]
In the server configuration, the get, set, and rewrite of parameters are supported. The supported parameters are as follows:

-|GET| SET
---|---|---
binlog-file-size	|o|	x
compact-cron	        |o|	o
compact-interval	|o|	o
compression	        |o|	x
daemonize	        |o|	x
db-path	                |o|	x
db-sync-path	        |o|	x
db-sync-speed	        |o|	x
double-master-ip	|o|	o
double-master-port	|o|	x
double-master-sid	|o|	x
dump-expire	        |o|	o
dump-path	        |o|	x
dump-prefix	        |o|	o
expire-logs-days	|o|	o
expire-logs-nums	|o|	o
identify-binlog-type	|o|	o
loglevel	        |o|	o
log-path	        |o|	x
masterauth	        |o|	o
max-background-compactions	|o|	x
max-background-flushes	        |o|	x
max-bytes-for-level-multiplier	|o|	x
max-cache-files	        |o|	x
maxclients              |o|	o
maxmemory	        |o|	x
network-interface	|o|	x
pidfile	                |o|	x
port	                |o|	x
requirepass	        |o|	o
root-connection-num	|o|	o
slaveof	                |o|	x
slave-priority	        |o|	o
slave-read-only	        |o|	o
slotmigrate	        |o(<3.0.0)|o(<3.0.0)
slowlog-log-slower-than	|o|	o
slowlog-write-errorlog  |o(<3.0.2)|o(<3.0.2)
sync-buffer-size	|o|	x
sync-thread-num	        |o|	x
target-file-size-base	|o|	x
thread-num	        |o|	x
timeout      	        |o|	o
userblacklist	        |o|	o
userpass	        |o|	o
write-buffer-size	|o|	x
max-cache-statistic-keys |o(<3.0.6)|o(<3.0.6)
small-compaction-threshold |o(<3.0.6)|o(<3.0.6)
databases               |o(<3.1.0)|x  
write-binlog            |o|     o
thread-pool-size        |o|     x
slowlog-max-len         |o|     o
share-block-cache       |o|     x
optimize-filters-for-hits  |o|  x
level-compaction-dynamic-level-bytes  |o|  x
cache-index-and-filter-blocks |o|  x
block-size              |o|     x
block-cache             |o|     x
sync-window-size        |o|	o



### purgelogsto [write2file-name]
`purgelogsto` is a Pika-original command for manually purging logs, similar to MySQL's `PURGE MASTER LOGS TO` command. This command has multiple validation mechanisms to ensure logs are safely purged.

### client list
Compared to Redis, the displayed information is less than Redis.

### client list order by [addr|idle]
A Pika-original command that sorts by IP address or connection idle time.

### client kill all
A Pika-original command that kills all current connections (excluding sync processes but including the current connection).

### Slow Log (slowlog)
Unlike Redis, Pika's slow log is not only stored in memory (accessible via the `slow log` command) but can also be stored in the error log without a count limit for easier analysis. This requires enabling the `slowlog-write-errorlog` parameter.

### bgsave
Similar to Redis's `bgsave`: first generates a snapshot, then backs up the snapshot data. Backup files are stored in the dump directory.

### dumpoff
Forcefully terminates an ongoing dump process (bgsave). After execution, backup stops immediately and a `dump-failed` folder is created in the dump directory (Deprecated from v2.0).

### delbackup
Deletes all snapshots in the dump directory except those currently in use (during full sync).

### compact
Immediately triggers a full compact operation on all data structures at the engine layer (RocksDB). A full compact merges SST files to eliminate deleted or expired data that has not been immediately cleaned up, reducing data size. Note that a full compact consumes some I/O resources.

### compact [string | hash | set | zset | list ]
Immediately triggers a full compact operation on the specified data structure at the engine layer (RocksDB). A compact on the specified structure merges SST files to eliminate deleted or expired data, reducing the data size for that structure. Note that a full compact consumes some I/O resources.

### compact $db [string | hash | set | zset | list ]
Performs a full compact on the specified DB. For example, `compact db0 all` performs a full compact on all data structures in db0.

### flushdb [string | hash | set | zset | list ]
The `flushdb` command allows clearing all data for a specified data structure only. To delete all data, use `flushall`.

### keys pattern [string | hash | set | zset | list ]
The `keys` command allows outputting all keys for a specified data structure only. To output all keys across all structures, do not use any parameters.

### slaveof ip port [write2file-name] [write2file-pos] [force]
The `slaveof` command allows specifying the write2file (binlog) file name and sync position for incremental sync. The `force` parameter triggers a forced full sync (useful when the master's write2file has been purged and cannot provide incremental sync to the slave). After full sync, Pika automatically switches to incremental sync.

### pkscanrange type key_start key_end [MATCH pattern] [LIMIT limit]
Performs a forward scan on the specified data structure, listing keys in the interval [key_start, key_end] (if type is string_with_value, it lists key-value pairs). ("", ""] represents the entire range.
* type: Specifies the data structure type to scan: {string_with_value | string | hash | list | zset | set}
* key_start: Starting key; empty string means -inf (negative infinity)
* key_end: Ending key; empty string means +inf (positive infinity)

### pkrscanrange type key_start key_end [MATCH pattern] [LIMIT limit]
Similar to pkscanrange, but in reverse order.

### pkhscanrange key field_start field_end  [MATCH pattern] [LIMIT limit]
Lists the field-value pairs in the interval [field_start, field_end] within the specified hash table.
* key: The key of the hash table.
* field_start: Starting field; empty string means -inf (negative infinity).
* field_end: Ending field; empty string means +inf (positive infinity).

### pkhrscanrange key field_start field_end  [MATCH pattern] [LIMIT limit]  
Similar to pkhscanrange, but in reverse order.

### diskrecovery 
A Pika-original command. When the disk is unexpectedly full, RocksDB enters write-protection mode. When sufficient space is made available, this command releases RocksDB's write-protection state, allowing writes to continue. This avoids the need to restart Pika after a disk-full event. Returns OK on success. If the current disk space is still insufficient, returns `"The available disk capacity is insufficient"`. No extra parameters are needed.
```bash
> diskrecovery 
> OK
```
### clearreplicationid
A Pika-original command that clears the Pika instance's replicationid and persists the change to the configuration file.
```bash
> clearreplicationid
> OK
```

### DisableWal
Controls the WAL write option using `disablewal (true/false)`. `true` disables WAL writes; `false` enables WAL writes. Note that WAL writes are enabled by default.
```bash
> disablewal true  // WAL write disabled
> OK
> disablewal false  // WAL write enabled
> OK
> disablewal asdfs // unrecognized parameter
> (error) ERR Invalid parameter
> disable false dasfasd // wrong number of arguments
> (error) ERR wrong number of arguments for 'disablewal' command
```
