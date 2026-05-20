Run the `INFO` command

```
127.0.0.1:9221> info [section]
```

```
# Master:
# Server
pika_version:2.3.0 -------------------------------------------- Pika version info
pika_git_sha:3668a2807a3d047ea43656b58a2130c1566eeb65 --------- Git SHA value
pika_build_compile_date: Nov 14 2017 -------------------------- Pika compilation date
os:Linux 2.6.32-2.0.0.8-6 x86_64 ------------------------------ Operating system info
arch_bits:64 -------------------------------------------------- OS architecture (bits)
process_id:12969 ---------------------------------------------- Pika PID info
tcp_port:9001 ------------------------------------------------- Pika port info
thread_num:12 ------------------------------------------------- Number of Pika threads
sync_thread_num:6 --------------------------------------------- Number of sync threads
uptime_in_seconds:3074 ---------------------------------------- Pika uptime (seconds)
uptime_in_days:0 ---------------------------------------------- Pika uptime (days)
config_file:/data1/pika9001/pika9001.conf --------------------- Pika conf file path
server_id:1 --------------------------------------------------- Pika server ID

# Data
db_size:770439 ------------------------------------------------ DB size (Bytes)
db_size_human:0M ---------------------------------------------- Human-readable DB size (MB)
compression:snappy -------------------------------------------- Compression algorithm
used_memory:4248 ---------------------------------------------- Memory used (Bytes)
used_memory_human:0M ------------------------------------------ Human-readable memory used (MB)
db_memtable_usage:4120 ---------------------------------------- Memtable usage (Bytes)
db_tablereader_usage:128 -------------------------------------- Table reader usage (Bytes)

# Log
log_size:110174 ----------------------------------------------- Binlog size (Bytes)
log_size_human:0M --------------------------------------------- Human-readable binlog size (MB)
safety_purge:none --------------------------------------------- Currently safely deletable file name
expire_logs_days:7 -------------------------------------------- Configured binlog expiry in days
expire_logs_nums:10 ------------------------------------------- Configured max binlog file count
binlog_offset:0 388 ------------------------------------------- Binlog offset (file number, offset)
 
# Clients
connected_clients:2 ------------------------------------------- Current connection count
 
# Stats
total_connections_received:18 --------------------------------- Total connection count
instantaneous_ops_per_sec:1 ----------------------------------- Current QPS
total_commands_processed:633 ---------------------------------- Total commands processed
is_bgsaving:No, , 0 ------------------------------------------- Backup info: is backing up, backup name, backup time
is_scaning_keyspace:No ---------------------------------------- Whether a keyspace scan is in progress
is_compact:No ------------------------------------------------- Whether compaction is in progress
compact_cron: ------------------------------------------------- Scheduled compact time window (format: start-end/ratio, e.g., 02-04/60)
compact_interval: --------------------------------------------- Compact interval (format: interval/ratio, e.g., 6/60)

# CPU
used_cpu_sys:48.52 -------------------------------------------- Pika process system CPU time
used_cpu_user:73.10 ------------------------------------------- Pika process user CPU time
used_cpu_sys_children:0.05 ------------------------------------ Pika child process system CPU time
used_cpu_user_children:0.05 ----------------------------------- Pika child process user CPU time
 
# Replication(MASTER)
role:master --------------------------------------------------- This instance's role
connected_slaves:1 -------------------------------------------- Number of connected slaves
slave0:ip=192.168.1.1,port=57765,state=online,sid=2,lag=0 ----- lag: bytes difference between master and slave binlog (byte); if multiple slaves, each is shown in sequence
 
# Slave (only the replication info section differs):
# Replication(SLAVE)
role:slave ---------------------------------------------------- This instance's role
master_host:192.168.1.2 --------------------------------------- Master IP
master_port:9001 ---------------------------------------------- Master port
master_link_status:up ----------------------------------------- Current sync status
slave_read_only:1 --------------------------------------------- Whether slave is read-only
repl_state: connected ----------------------------------------- Current state of slave sync connection
 
# Keyspace (key count display, categorized by data type; not updated by default; only refreshed when "info keyspace 1" is executed)
# Time:2016-04-22 17:08:33 ------------------------------------ Time of last statistics
db0 Strings_keys=100004, expires=0, invaild_keys=0
db0 Hashes_keys=2, expires=0, invaild_keys=0
db0 Lists_keys=0, expires=0, invaild_keys=0
db0 Zsets_keys=1, expires=0, invaild_keys=0
db0 Sets_keys=0, expires=0, invaild_keys=0
# keys: current count of valid keys, equivalent to Redis's keys
# expires: count of keys with an expire attribute, equivalent to Redis
# invalid_keys: Pika-specific; keys that have been invalidated (marked for deletion) but not yet physically deleted by RocksDB. Although these keys are no longer accessible, they still occupy some disk space. If the count is high, run compact to fully clean them up.

# DoubleMaster(MASTER)
role:master --------------------------------------------------- Dual-master role
the peer-master host: ----------------------------------------- Dual-master peer IP
the peer-master port:0 ---------------------------------------- Dual-master peer port
the peer-master server_id:0 ----------------------------------- Dual-master peer server ID
double_master_mode: False ------------------------------------- Whether dual-master mode is configured
repl_state: 0 ------------------------------------------------- Dual-master connection state
double_master_recv_info: filenum 0 offset 0 ------------------- Binlog offset received from peer
```
