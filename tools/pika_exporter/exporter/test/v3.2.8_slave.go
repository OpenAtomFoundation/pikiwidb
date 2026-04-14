package test

// V328SlaveInfo represents Pika 3.2.8 slave instance info
var V328SlaveInfo = `# Server
pika_version:3.2.8
pika_git_sha:f6a355ac56c8c439ecca53c3a6c3a159ef3da90a
pika_build_compile_date: Dec 20 2019
os:Linux 4.19.91-27.2.an8.x86_64 x86_64
arch_bits:64
process_id:1587109
tcp_port:8850
thread_num:12
sync_thread_num:12
uptime_in_seconds:15840979
uptime_in_days:184
config_file:/data1/pika8850/pika8850.conf
server_id:1

# Data
db_size:110987871832
db_size_human:105846M
log_size:117601826
log_size_human:112M
compression:snappy
used_memory:982875737
used_memory_human:937M
db_memtable_usage:23029184
db_tablereader_usage:959846553
db_fatal:0
db_fatal_msg:NULL

# Clients
connected_clients:739

# Stats
total_connections_received:63065
instantaneous_ops_per_sec:87
total_commands_processed:5951615
is_bgsaving:No
is_scaning_keyspace:No
is_compact:No
compact_cron:
compact_interval:

# CPU
used_cpu_sys:397392.81
used_cpu_user:143773.08
used_cpu_sys_children:0.00
used_cpu_user_children:0.00

# Replication(SLAVE)
role:slave
master_host:10.175.131.182
master_port:8850
master_link_status:up
slave_priority:0
slave_read_only:1
db0 binlog_offset=14275 9267088,safety_purge=write2file14265

# Keyspace
# Time:1970-01-01 08:00:00
db0 Strings_keys=0, expires=0, invaild_keys=0
db0 Hashes_keys=0, expires=0, invaild_keys=0
db0 Lists_keys=0, expires=0, invaild_keys=0
db0 Zsets_keys=0, expires=0, invaild_keys=0
db0 Sets_keys=0, expires=0, invaild_keys=0`
