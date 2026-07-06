## client kill all
Delete all clients.

```
xxx.qihoo.net:8221> client kill all
OK
xxx.qihoo.net:8221>
```

## bgsave
Execution is the same as Redis. After asynchronous dump is complete, the database is saved under the dump_path directory; the dumped database name includes dump_prefix and dump time information.

```
xxx.qihoo.net:8221> BGSAVE
20160422134755 : 2213: 32582292
```
The returned information includes the dump time (20160422134755) and the current binlog position, i.e., file number: offset (2213: 32582292).
    
```
xxx.qihoo.net # ll /data3/pika_test/dump/
Total 0
drwxr-xr-x 1 root root 42 Apr 22 13:47 pika8221-20160422
```
"/data3/pika_test/dump/" is the dump path, "pika9221-" is the dump_prefix, and 20160422 is the dump date.

## delbackup
Deletes all snapshots in the dump directory except those currently in use (during full sync).

```
xxx.qihoo.net:8221> DELBACKUP
OK
```

## info keyspace
Executed as "info keyspace 1", "info keyspace 0", or "info keyspace". "info keyspace" and "info keyspace 0" are equivalent.  
info keyspace 1: Asynchronously starts a keyspace scan and returns the result from the last completed scan.  
info keyspace 0: Returns the result from the last completed keyspace scan.  

```
xxx.qihoo.net:8221> info keyspace 1
# Keyspace
# Time:1970-01-01 08:00:00
kv keys:0
hash keys:0
list keys:0
zset keys:0
set keys:0
xxx.qihoo.net:8221> info keyspace
# Keyspace
# Time:2016-04-22 13:45:54
kv keys:13
hash keys:0
list keys:0
zset keys:0
set keys:0
```
 
## config get/set *
The usage of `config get` and `config set` is the same as Redis, but the options may differ. Two commands are provided:  
1) config get *  
2) config set *  
These list the options that `config get` and `config set` can operate on respectively.

```
xxx.qihoo.net:8221> config get *
 1) "port"
 2) "thread_num"
 3) "log_path"
 4) "log_level"
 5) "db_path"
 6) "maxmemory"
 7) "write_buffer_size"
 8) "timeout"
 9) "requirepass"
10) "userpass"
11) "userblacklist"
12) "daemonize"
13) "dump_path"
14) "dump_prefix"
15) "pidfile"
16) "maxconnection"
17) "target_file_size_base"
18) "expire_logs_days"
19) "expire_logs_nums"
20) "root_connection_num"
21) "slowlog_log_slower_than"
22) "slave-read-only"
23) "binlog_file_size"
24) "compression"
25) "db_sync_path"
26) "db_sync_speed"
xxx.qihoo.net:8221> config set *
 1) "log_level"
 2) "timeout"
 3) "requirepass"
 4) "userpass"
 5) "userblacklist"
 6) "dump_prefix"
 7) "maxconnection"
 8) "expire_logs_days"
 9) "expire_logs_nums"
10) "root_connection_num"
11) "slowlog_log_slower_than"
12) "slave-read-only"
13) "db_sync_speed"
bada06.add.zwt.qihoo.net:8221>
```
## compact
Since Pika's underlying storage engine is based on a modified RocksDB, there are read amplification, write amplification, and space amplification issues. In addition to RocksDB's automatic compaction, Pika also provides a manual compaction command to forcefully compact the entire keyspace. It supports compacting a single data structure. Syntax: `compact [string/hash/set/zset/list/all]`.

```
xxx.qihoo.net:8221> compact
OK
```
When the keyspace is large, executing the `compact` command can significantly reduce occupied space. However, it takes a long time and affects read/write performance, so it is recommended to execute it during low-traffic periods.

## readonly (Deprecated after version 3.1)
This command controls the server's write permission. Execution methods:
1) "readonly on"  
2) "readonly off"  
3) "readonly 1"   
4) "readonly 0"  
1) and 3) are equivalent; 2) and 4) are equivalent.

```
xxx.qihoo.net:8221> set a b
OK
xxx.qihoo.net:8221> get a
"b"
xxx.qihoo.net:8221> readonly 1
OK
xxx.qihoo.net:8221> set a c
(error) ERR Server in read-only
xxx.qihoo.net:8221> get a
"b"
xxx.qihoo.net:8221> readonly 0
OK
xxx.qihoo.net:8221> set a c
OK
xxx.qihoo.net:8221> get a
"c"
xxx.qihoo.net:8221>
```
