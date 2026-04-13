package test

// V336MasterInfo represents Pika 3.3.6 master instance info
var V336MasterInfo = `# Server
pika_version:3.3.6
pika_git_sha:9e74c8cd0040a0a63c35e9d426c7d3b6464b378e
pika_build_compile_date: Dec  4 2020
os:Linux 4.19.49-1.el7.x86_64 x86_64
arch_bits:64
process_id:42694
tcp_port:26245
thread_num:20
sync_thread_num:12
uptime_in_seconds:159960482
uptime_in_days:1852
config_file:/data1/pika26245/pika26245.conf
server_id:1

# Data
db_size:85239430517
db_size_human:81290M
log_size:949684206
log_size_human:905M
compression:snappy
used_memory:1260691892
used_memory_human:1202M
db_memtable_usage:156492328
db_tablereader_usage:1104199564
db_fatal:0
db_fatal_msg:NULL

# Clients
connected_clients:2881

# Stats
total_connections_received:80071
instantaneous_ops_per_sec:324
total_commands_processed:21756528
is_bgsaving:No
is_scaning_keyspace:No
is_compact:No
compact_cron:
compact_interval:

# Command_Exec_Count
SCAN:1
INFO:32649709
SELECT:3977197
PING:25459644293
AUTH:41106287
GET:2245849546
SET:244940320
MONITOR:1
CONFIG:21816695
SLOWLOG:553706
SETEX:8

# CPU
used_cpu_sys:8039902.50
used_cpu_user:2010937.25
used_cpu_sys_children:0.00
used_cpu_user_children:0.00

# Replication(MASTER)
role:master
connected_slaves:5
slave0:ip=10.218.51.8,port=26245,conn_fd=1181,lag=(db0:0)
db0 binlog_offset=1690 273344,safety_purge=write2file1680

# Keyspace
# Time:1970-01-01 08:00:00
db0 Strings_keys=0, expires=0, invalid_keys=0
db0 Hashes_keys=0, expires=0, invalid_keys=0
db0 Lists_keys=0, expires=0, invalid_keys=0
db0 Zsets_keys=0, expires=0, invalid_keys=0
db0 Sets_keys=0, expires=0, invalid_keys=0`
