package test

// V328MasterInfo represents Pika 3.2.8 master instance info
var V328MasterInfo = `# Server
pika_version:3.2.8
pika_git_sha:f6a355ac56c8c439ecca53c3a6c3a159ef3da90a
pika_build_compile_date: Dec 20 2019
os:Linux 4.19.49-1.el7.x86_64 x86_64
arch_bits:64
process_id:40310
tcp_port:8850
thread_num:12
sync_thread_num:12
uptime_in_seconds:173148695
uptime_in_days:2005
config_file:/data1/pika8850/pika8850.conf
server_id:1

# Data
db_size:110944960930
db_size_human:105805M
log_size:957508433
log_size_human:913M
compression:snappy
used_memory:981717521
used_memory_human:936M
db_memtable_usage:22225696
db_tablereader_usage:959491825
db_fatal:0
db_fatal_msg:NULL

# Clients
connected_clients:1441

# Stats
total_connections_received:79121
instantaneous_ops_per_sec:167
total_commands_processed:11591054
is_bgsaving:No
is_scaning_keyspace:No
is_compact:No
compact_cron:
compact_interval:

# CPU
used_cpu_sys:4510111.00
used_cpu_user:1665861.38
used_cpu_sys_children:1302.07
used_cpu_user_children:2768.07

# Replication(MASTER)
role:master
connected_slaves:5
slave0:ip=10.175.13.76,port=8850,conn_fd=3623,lag=(db0:0)
db0 binlog_offset=14275 9266464,safety_purge=write2file14265

# Keyspace
# Time:1970-01-01 08:00:00
db0 Strings_keys=0, expires=0, invaild_keys=0
db0 Hashes_keys=0, expires=0, invaild_keys=0
db0 Lists_keys=0, expires=0, invaild_keys=0
db0 Zsets_keys=0, expires=0, invaild_keys=0
db0 Sets_keys=0, expires=0, invaild_keys=0`
