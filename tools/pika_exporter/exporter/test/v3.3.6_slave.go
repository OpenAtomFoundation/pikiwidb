package test

// V336SlaveInfo represents Pika 3.3.6 slave instance info
var V336SlaveInfo = `# Server
pika_version:3.3.6
pika_git_sha:9e74c8cd0040a0a63c35e9d426c7d3b6464b378e
pika_build_compile_date: Dec  4 2020
os:Linux 4.19.49-1.el7.x86_64 x86_64
arch_bits:64
process_id:24125
tcp_port:26245
thread_num:20
sync_thread_num:12
uptime_in_seconds:159960531
uptime_in_days:1852
config_file:/data1/pika26245/pika26245.conf
server_id:1

# Data
db_size:85074567821
db_size_human:81133M
log_size:954638636
log_size_human:910M
compression:snappy
used_memory:1268591215
used_memory_human:1209M
db_memtable_usage:166536376
db_tablereader_usage:1102054839
db_fatal:0
db_fatal_msg:NULL

# Clients
connected_clients:1

# Stats
total_connections_received:63556
instantaneous_ops_per_sec:0
total_commands_processed:65292
is_bgsaving:No
is_scaning_keyspace:No
is_compact:No
compact_cron:
compact_interval:

# CPU
used_cpu_sys:6318819.00
used_cpu_user:1798476.12
used_cpu_sys_children:0.00
used_cpu_user_children:0.00

# Replication(SLAVE)
role:slave
master_host:10.175.131.136
master_port:26245
master_link_status:up
slave_priority:32362144
slave_read_only:1
db0 binlog_offset=1690 273448,safety_purge=write2file1680

# Keyspace
# Time:1970-01-01 08:00:00
db0 Strings_keys=0, expires=0, invalid_keys=0
db0 Hashes_keys=0, expires=0, invalid_keys=0
db0 Lists_keys=0, expires=0, invalid_keys=0
db0 Zsets_keys=0, expires=0, invalid_keys=0
db0 Sets_keys=0, expires=0, invalid_keys=0`
