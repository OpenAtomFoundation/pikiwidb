package test

var V355SlaveInfo = `# Server
pika_version:3.5.5
pika_git_sha:29a7629bc97c237531f12689b92cb59ee73bdef7
pika_build_compile_date: 2025-07-30 10:25:07
os:Linux 4.19.91-27.2.an8.x86_64 x86_64
arch_bits:64
process_id:275922
tcp_port:5436
thread_num:4
sync_thread_num:12
sync_binlog_thread_num:1
uptime_in_seconds:9172404
uptime_in_days:107
config_file:/data1/pika5436/pika5436.conf
server_id:1
run_id:a9af427fc92e4157aad5882e9994bfc7cecd1b84

# Data
db_size:22764188213
db_size_human:21709M
log_size:46657816
log_size_human:44M
compression:snappy
used_memory:50745234
used_memory_human:48M
db_memtable_usage:33579008
db_tablereader_usage:17166226
db_fatal:0
db_fatal_msg:nullptr

# Clients
connected_clients:1

# Stats
total_connections_received:27
instantaneous_ops_per_sec:0
total_commands_processed:1233
total_net_input_bytes:22795269059
total_net_output_bytes:279570645
total_net_repl_input_bytes:22795054435
total_net_repl_output_bytes:274459504
instantaneous_input_kbps:0.0917969
instantaneous_output_kbps:0.369141
instantaneous_input_repl_kbps:0.0849609
instantaneous_output_repl_kbps:0.140625
is_bgsaving:No
is_scaning_keyspace:No
is_compact:No
compact_cron:
compact_interval:
is_slots_reloading:No, , 0
is_slots_cleaningup:No, , 0
is_slots_migrating:No, , 0
slow_logs_count:0

# Command_Exec_Count
SET:193040
INFO:3383
SLAVEOF:1
AUTH:2877
MONITOR:11
SCAN:15
CONFIG:107

# Commandstats
slaveof:calls=1, usec=0.75, usec_per_call=0.75
info:calls=3382, usec=596.86, usec_per_call=0.18
monitor:calls=11, usec=0.20, usec_per_call=0.02
auth:calls=2877, usec=104.36, usec_per_call=0.04
scan:calls=15, usec=2.96, usec_per_call=0.20
config:calls=107, usec=1.14, usec_per_call=0.01

# Cache
cache_status:Ok
cache_db_num:8
cache_keys:0
cache_memory:39232
cache_memory_human:0M
hits:0
all_cmds:0
hits_per_sec:0
read_cmd_per_sec:0
hitratio_per_sec:0%
hitratio_all:0%
load_keys_per_sec:0
waitting_load_keys_num:0

# CPU
used_cpu_sys:1115.15
used_cpu_user:724.95
used_cpu_sys_children:0.00
used_cpu_user_children:0.00

# Replication(SLAVE)
role:slave
ReplicationID:40174f033ff85653ac7fd3ec4d9de8f33c2e21e95bd1262970
master_host:10.243.48.117
master_port:5436
master_link_status:up
repl_connect_status:
db0:connected
slave_priority:100
slave_read_only:1
is_eligible_for_master_election:true
db0:binlog_offset=368 42825224,safety_purge=write2file358
slave_repl_offset:38630422024

# Keyspace
# Start async statistics
# Time:0
db0 Strings_keys=0, expires=0, invalid_keys=0
db0 Hashes_keys=0, expires=0, invalid_keys=0
db0 Lists_keys=0, expires=0, invalid_keys=0
db0 Zsets_keys=0, expires=0, invalid_keys=0
db0 Sets_keys=0, expires=0, invalid_keys=0


# RocksDB
#strings_RocksDB
strings_num_immutable_mem_table:0
strings_num_immutable_mem_table_flushed:0
strings_mem_table_flush_pending:0
strings_num_running_flushes:0
strings_compaction_pending:0
strings_num_running_compactions:0
strings_background_errors:0
strings_cur_size_active_mem_table:33556480
strings_cur_size_all_mem_tables:33556480
strings_size_all_mem_tables:33556480
strings_estimate_num_keys:16150427
strings_estimate_table_readers_mem:1030282
strings_num_snapshots:0
strings_num_live_versions:1
strings_current_super_version_number:1
strings_estimate_live_data_size:21510423320
strings_total_sst_files_size:21673347794
strings_live_sst_files_size:21673347794
strings_estimate_pending_compaction_bytes:0
strings_block_cache_capacity:2147483648
strings_block_cache_usage:96
strings_block_cache_pinned_usage:96
strings_num_blob_files:0
strings_blob_stats:1
strings_total_blob_file_size:0
strings_live_blob_file_size:0
strings_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
strings_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
strings_compaction.L0.AvgSec: 0.000000
strings_compaction.L0.CompCount: 0.000000
strings_compaction.L0.CompMergeCPU: 0.000000
strings_compaction.L0.CompSec: 0.000000
strings_compaction.L0.CompactedFiles: 0.000000
strings_compaction.L0.KeyDrop: 0.000000
strings_compaction.L0.KeyIn: 0.000000
strings_compaction.L0.MovedGB: 0.000000
strings_compaction.L0.NumFiles: 1.000000
strings_compaction.L0.RblobGB: 0.000000
strings_compaction.L0.ReadGB: 0.000000
strings_compaction.L0.ReadMBps: 0.000000
strings_compaction.L0.RnGB: 0.000000
strings_compaction.L0.Rnp1GB: 0.000000
strings_compaction.L0.Score: 0.250000
strings_compaction.L0.SizeBytes: 1291.000000
strings_compaction.L0.WblobGB: 0.000000
strings_compaction.L0.WnewGB: 0.000000
strings_compaction.L0.WriteAmp: 0.000000
strings_compaction.L0.WriteGB: 0.000000
strings_compaction.L0.WriteMBps: 0.000000
strings_compaction.L4.AvgSec: 0.000000
strings_compaction.L4.CompCount: 0.000000
strings_compaction.L4.CompMergeCPU: 0.000000
strings_compaction.L4.CompSec: 0.000000
strings_compaction.L4.CompactedFiles: 0.000000
strings_compaction.L4.KeyDrop: 0.000000
strings_compaction.L4.KeyIn: 0.000000
strings_compaction.L4.MovedGB: 0.000000
strings_compaction.L4.NumFiles: 14.000000
strings_compaction.L4.RblobGB: 0.000000
strings_compaction.L4.ReadGB: 0.000000
strings_compaction.L4.ReadMBps: 0.000000
strings_compaction.L4.RnGB: 0.000000
strings_compaction.L4.Rnp1GB: 0.000000
strings_compaction.L4.Score: 0.927166
strings_compaction.L4.SizeBytes: 248884307.000000
strings_compaction.L4.WblobGB: 0.000000
strings_compaction.L4.WnewGB: 0.000000
strings_compaction.L4.WriteAmp: 0.000000
strings_compaction.L4.WriteGB: 0.000000
strings_compaction.L4.WriteMBps: 0.000000
strings_compaction.L5.AvgSec: 0.000000
strings_compaction.L5.CompCount: 0.000000
strings_compaction.L5.CompMergeCPU: 0.000000
strings_compaction.L5.CompSec: 0.000000
strings_compaction.L5.CompactedFiles: 0.000000
strings_compaction.L5.KeyDrop: 0.000000
strings_compaction.L5.KeyIn: 0.000000
strings_compaction.L5.MovedGB: 0.000000
strings_compaction.L5.NumFiles: 89.000000
strings_compaction.L5.RblobGB: 0.000000
strings_compaction.L5.ReadGB: 0.000000
strings_compaction.L5.ReadMBps: 0.000000
strings_compaction.L5.RnGB: 0.000000
strings_compaction.L5.Rnp1GB: 0.000000
strings_compaction.L5.Score: 0.989415
strings_compaction.L5.SizeBytes: 1928919001.000000
strings_compaction.L5.WblobGB: 0.000000
strings_compaction.L5.WnewGB: 0.000000
strings_compaction.L5.WriteAmp: 0.000000
strings_compaction.L5.WriteGB: 0.000000
strings_compaction.L5.WriteMBps: 0.000000
strings_compaction.L6.AvgSec: 0.000000
strings_compaction.L6.CompCount: 0.000000
strings_compaction.L6.CompMergeCPU: 0.000000
strings_compaction.L6.CompSec: 0.000000
strings_compaction.L6.CompactedFiles: 0.000000
strings_compaction.L6.KeyDrop: 0.000000
strings_compaction.L6.KeyIn: 0.000000
strings_compaction.L6.MovedGB: 0.000000
strings_compaction.L6.NumFiles: 877.000000
strings_compaction.L6.RblobGB: 0.000000
strings_compaction.L6.ReadGB: 0.000000
strings_compaction.L6.ReadMBps: 0.000000
strings_compaction.L6.RnGB: 0.000000
strings_compaction.L6.Rnp1GB: 0.000000
strings_compaction.L6.Score: 0.000000
strings_compaction.L6.SizeBytes: 19495543195.000000
strings_compaction.L6.WblobGB: 0.000000
strings_compaction.L6.WnewGB: 0.000000
strings_compaction.L6.WriteAmp: 0.000000
strings_compaction.L6.WriteGB: 0.000000
strings_compaction.L6.WriteMBps: 0.000000
strings_compaction.Sum.AvgSec: 0.000000
strings_compaction.Sum.CompCount: 0.000000
strings_compaction.Sum.CompMergeCPU: 0.000000
strings_compaction.Sum.CompSec: 0.000000
strings_compaction.Sum.CompactedFiles: 0.000000
strings_compaction.Sum.KeyDrop: 0.000000
strings_compaction.Sum.KeyIn: 0.000000
strings_compaction.Sum.MovedGB: 0.000000
strings_compaction.Sum.NumFiles: 981.000000
strings_compaction.Sum.RblobGB: 0.000000
strings_compaction.Sum.ReadGB: 0.000000
strings_compaction.Sum.ReadMBps: 0.000000
strings_compaction.Sum.RnGB: 0.000000
strings_compaction.Sum.Rnp1GB: 0.000000
strings_compaction.Sum.Score: 0.000000
strings_compaction.Sum.SizeBytes: 21673347794.000000
strings_compaction.Sum.WblobGB: 0.000000
strings_compaction.Sum.WnewGB: 0.000000
strings_compaction.Sum.WriteAmp: 0.000000
strings_compaction.Sum.WriteGB: 0.000000
strings_compaction.Sum.WriteMBps: 0.000000
strings_l0-file-count-limit-delays: 0
strings_l0-file-count-limit-stops: 0
strings_memtable-limit-delays: 0
strings_memtable-limit-stops: 0
strings_pending-compaction-bytes-delays: 0
strings_pending-compaction-bytes-stops: 0
strings_total-delays: 0
strings_total-stops: 0
#hashes_RocksDB
hashes_num_immutable_mem_table:0
hashes_num_immutable_mem_table_flushed:0
hashes_mem_table_flush_pending:0
hashes_num_running_flushes:0
hashes_compaction_pending:0
hashes_num_running_compactions:0
hashes_background_errors:0
hashes_cur_size_active_mem_table:4096
hashes_cur_size_all_mem_tables:4096
hashes_size_all_mem_tables:4096
hashes_estimate_num_keys:307528
hashes_estimate_table_readers_mem:552728
hashes_num_snapshots:0
hashes_num_live_versions:2
hashes_current_super_version_number:2
hashes_estimate_live_data_size:14394569
hashes_total_sst_files_size:14394569
hashes_live_sst_files_size:14394569
hashes_estimate_pending_compaction_bytes:0
hashes_block_cache_capacity:4294967296
hashes_block_cache_usage:192
hashes_block_cache_pinned_usage:192
hashes_num_blob_files:0
hashes_blob_stats:22122264
hashes_total_blob_file_size:0
hashes_live_blob_file_size:0
hashes_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
hashes_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
hashes_compaction.L0.AvgSec: 0.000000
hashes_compaction.L0.CompCount: 0.000000
hashes_compaction.L0.CompMergeCPU: 0.000000
hashes_compaction.L0.CompSec: 0.000000
hashes_compaction.L0.CompactedFiles: 0.000000
hashes_compaction.L0.KeyDrop: 0.000000
hashes_compaction.L0.KeyIn: 0.000000
hashes_compaction.L0.MovedGB: 0.000000
hashes_compaction.L0.NumFiles: 1.000000
hashes_compaction.L0.RblobGB: 0.000000
hashes_compaction.L0.ReadGB: 0.000000
hashes_compaction.L0.ReadMBps: 0.000000
hashes_compaction.L0.RnGB: 0.000000
hashes_compaction.L0.Rnp1GB: 0.000000
hashes_compaction.L0.Score: 0.250000
hashes_compaction.L0.SizeBytes: 3467349.000000
hashes_compaction.L0.WblobGB: 0.000000
hashes_compaction.L0.WnewGB: 0.000000
hashes_compaction.L0.WriteAmp: 0.000000
hashes_compaction.L0.WriteGB: 0.000000
hashes_compaction.L0.WriteMBps: 0.000000
hashes_compaction.Sum.AvgSec: 0.000000
hashes_compaction.Sum.CompCount: 0.000000
hashes_compaction.Sum.CompMergeCPU: 0.000000
hashes_compaction.Sum.CompSec: 0.000000
hashes_compaction.Sum.CompactedFiles: 0.000000
hashes_compaction.Sum.KeyDrop: 0.000000
hashes_compaction.Sum.KeyIn: 0.000000
hashes_compaction.Sum.MovedGB: 0.000000
hashes_compaction.Sum.NumFiles: 1.000000
hashes_compaction.Sum.RblobGB: 0.000000
hashes_compaction.Sum.ReadGB: 0.000000
hashes_compaction.Sum.ReadMBps: 0.000000
hashes_compaction.Sum.RnGB: 0.000000
hashes_compaction.Sum.Rnp1GB: 0.000000
hashes_compaction.Sum.Score: 0.000000
hashes_compaction.Sum.SizeBytes: 3467349.000000
hashes_compaction.Sum.WblobGB: 0.000000
hashes_compaction.Sum.WnewGB: 0.000000
hashes_compaction.Sum.WriteAmp: 0.000000
hashes_compaction.Sum.WriteGB: 0.000000
hashes_compaction.Sum.WriteMBps: 0.000000
hashes_l0-file-count-limit-delays: 0
hashes_l0-file-count-limit-stops: 0
hashes_memtable-limit-delays: 0
hashes_memtable-limit-stops: 0
hashes_pending-compaction-bytes-delays: 0
hashes_pending-compaction-bytes-stops: 0
hashes_total-delays: 0
hashes_total-stops: 0
#lists_RocksDB
lists_num_immutable_mem_table:0
lists_num_immutable_mem_table_flushed:0
lists_mem_table_flush_pending:0
lists_num_running_flushes:0
lists_compaction_pending:0
lists_num_running_compactions:0
lists_background_errors:0
lists_cur_size_active_mem_table:4096
lists_cur_size_all_mem_tables:4096
lists_size_all_mem_tables:4096
lists_estimate_num_keys:165482
lists_estimate_table_readers_mem:1275679
lists_num_snapshots:0
lists_num_live_versions:2
lists_current_super_version_number:2
lists_estimate_live_data_size:123538200
lists_total_sst_files_size:123538200
lists_live_sst_files_size:123538200
lists_estimate_pending_compaction_bytes:0
lists_block_cache_capacity:4294967296
lists_block_cache_usage:192
lists_block_cache_pinned_usage:192
lists_num_blob_files:0
lists_blob_stats:22122264
lists_total_blob_file_size:0
lists_live_blob_file_size:0
lists_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
lists_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
lists_compaction.L0.AvgSec: 0.000000
lists_compaction.L0.CompCount: 0.000000
lists_compaction.L0.CompMergeCPU: 0.000000
lists_compaction.L0.CompSec: 0.000000
lists_compaction.L0.CompactedFiles: 0.000000
lists_compaction.L0.KeyDrop: 0.000000
lists_compaction.L0.KeyIn: 0.000000
lists_compaction.L0.MovedGB: 0.000000
lists_compaction.L0.NumFiles: 1.000000
lists_compaction.L0.RblobGB: 0.000000
lists_compaction.L0.ReadGB: 0.000000
lists_compaction.L0.ReadMBps: 0.000000
lists_compaction.L0.RnGB: 0.000000
lists_compaction.L0.Rnp1GB: 0.000000
lists_compaction.L0.Score: 0.250000
lists_compaction.L0.SizeBytes: 842452.000000
lists_compaction.L0.WblobGB: 0.000000
lists_compaction.L0.WnewGB: 0.000000
lists_compaction.L0.WriteAmp: 0.000000
lists_compaction.L0.WriteGB: 0.000000
lists_compaction.L0.WriteMBps: 0.000000
lists_compaction.Sum.AvgSec: 0.000000
lists_compaction.Sum.CompCount: 0.000000
lists_compaction.Sum.CompMergeCPU: 0.000000
lists_compaction.Sum.CompSec: 0.000000
lists_compaction.Sum.CompactedFiles: 0.000000
lists_compaction.Sum.KeyDrop: 0.000000
lists_compaction.Sum.KeyIn: 0.000000
lists_compaction.Sum.MovedGB: 0.000000
lists_compaction.Sum.NumFiles: 1.000000
lists_compaction.Sum.RblobGB: 0.000000
lists_compaction.Sum.ReadGB: 0.000000
lists_compaction.Sum.ReadMBps: 0.000000
lists_compaction.Sum.RnGB: 0.000000
lists_compaction.Sum.Rnp1GB: 0.000000
lists_compaction.Sum.Score: 0.000000
lists_compaction.Sum.SizeBytes: 842452.000000
lists_compaction.Sum.WblobGB: 0.000000
lists_compaction.Sum.WnewGB: 0.000000
lists_compaction.Sum.WriteAmp: 0.000000
lists_compaction.Sum.WriteGB: 0.000000
lists_compaction.Sum.WriteMBps: 0.000000
lists_l0-file-count-limit-delays: 0
lists_l0-file-count-limit-stops: 0
lists_memtable-limit-delays: 0
lists_memtable-limit-stops: 0
lists_pending-compaction-bytes-delays: 0
lists_pending-compaction-bytes-stops: 0
lists_total-delays: 0
lists_total-stops: 0
#sets_RocksDB
sets_num_immutable_mem_table:0
sets_num_immutable_mem_table_flushed:0
sets_mem_table_flush_pending:0
sets_num_running_flushes:0
sets_compaction_pending:0
sets_num_running_compactions:0
sets_background_errors:0
sets_cur_size_active_mem_table:4096
sets_cur_size_all_mem_tables:4096
sets_size_all_mem_tables:4096
sets_estimate_num_keys:3406554
sets_estimate_table_readers_mem:11285316
sets_num_snapshots:0
sets_num_live_versions:2
sets_current_super_version_number:2
sets_estimate_live_data_size:759058158
sets_total_sst_files_size:759058158
sets_live_sst_files_size:759058158
sets_estimate_pending_compaction_bytes:0
sets_block_cache_capacity:4294967296
sets_block_cache_usage:192
sets_block_cache_pinned_usage:192
sets_num_blob_files:0
sets_blob_stats:22122264
sets_total_blob_file_size:0
sets_live_blob_file_size:0
sets_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
sets_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
sets_compaction.L0.AvgSec: 0.000000
sets_compaction.L0.CompCount: 0.000000
sets_compaction.L0.CompMergeCPU: 0.000000
sets_compaction.L0.CompSec: 0.000000
sets_compaction.L0.CompactedFiles: 0.000000
sets_compaction.L0.KeyDrop: 0.000000
sets_compaction.L0.KeyIn: 0.000000
sets_compaction.L0.MovedGB: 0.000000
sets_compaction.L0.NumFiles: 1.000000
sets_compaction.L0.RblobGB: 0.000000
sets_compaction.L0.ReadGB: 0.000000
sets_compaction.L0.ReadMBps: 0.000000
sets_compaction.L0.RnGB: 0.000000
sets_compaction.L0.Rnp1GB: 0.000000
sets_compaction.L0.Score: 0.250000
sets_compaction.L0.SizeBytes: 69453.000000
sets_compaction.L0.WblobGB: 0.000000
sets_compaction.L0.WnewGB: 0.000000
sets_compaction.L0.WriteAmp: 0.000000
sets_compaction.L0.WriteGB: 0.000000
sets_compaction.L0.WriteMBps: 0.000000
sets_compaction.Sum.AvgSec: 0.000000
sets_compaction.Sum.CompCount: 0.000000
sets_compaction.Sum.CompMergeCPU: 0.000000
sets_compaction.Sum.CompSec: 0.000000
sets_compaction.Sum.CompactedFiles: 0.000000
sets_compaction.Sum.KeyDrop: 0.000000
sets_compaction.Sum.KeyIn: 0.000000
sets_compaction.Sum.MovedGB: 0.000000
sets_compaction.Sum.NumFiles: 1.000000
sets_compaction.Sum.RblobGB: 0.000000
sets_compaction.Sum.ReadGB: 0.000000
sets_compaction.Sum.ReadMBps: 0.000000
sets_compaction.Sum.RnGB: 0.000000
sets_compaction.Sum.Rnp1GB: 0.000000
sets_compaction.Sum.Score: 0.000000
sets_compaction.Sum.SizeBytes: 69453.000000
sets_compaction.Sum.WblobGB: 0.000000
sets_compaction.Sum.WnewGB: 0.000000
sets_compaction.Sum.WriteAmp: 0.000000
sets_compaction.Sum.WriteGB: 0.000000
sets_compaction.Sum.WriteMBps: 0.000000
sets_l0-file-count-limit-delays: 0
sets_l0-file-count-limit-stops: 0
sets_memtable-limit-delays: 0
sets_memtable-limit-stops: 0
sets_pending-compaction-bytes-delays: 0
sets_pending-compaction-bytes-stops: 0
sets_total-delays: 0
sets_total-stops: 0
#zsets_RocksDB
zsets_num_immutable_mem_table:0
zsets_num_immutable_mem_table_flushed:0
zsets_mem_table_flush_pending:0
zsets_num_running_flushes:0
zsets_compaction_pending:0
zsets_num_running_compactions:0
zsets_background_errors:0
zsets_cur_size_active_mem_table:6144
zsets_cur_size_all_mem_tables:6144
zsets_size_all_mem_tables:6144
zsets_estimate_num_keys:2011867
zsets_estimate_table_readers_mem:3022221
zsets_num_snapshots:0
zsets_num_live_versions:3
zsets_current_super_version_number:3
zsets_estimate_live_data_size:48903360
zsets_total_sst_files_size:48903360
zsets_live_sst_files_size:48903360
zsets_estimate_pending_compaction_bytes:0
zsets_block_cache_capacity:6442450944
zsets_block_cache_usage:288
zsets_block_cache_pinned_usage:288
zsets_num_blob_files:0
zsets_blob_stats:140082180096088
zsets_total_blob_file_size:0
zsets_live_blob_file_size:0
zsets_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
zsets_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
zsets_compaction.L0.AvgSec: 0.000000
zsets_compaction.L0.CompCount: 0.000000
zsets_compaction.L0.CompMergeCPU: 0.000000
zsets_compaction.L0.CompSec: 0.000000
zsets_compaction.L0.CompactedFiles: 0.000000
zsets_compaction.L0.KeyDrop: 0.000000
zsets_compaction.L0.KeyIn: 0.000000
zsets_compaction.L0.MovedGB: 0.000000
zsets_compaction.L0.NumFiles: 1.000000
zsets_compaction.L0.RblobGB: 0.000000
zsets_compaction.L0.ReadGB: 0.000000
zsets_compaction.L0.ReadMBps: 0.000000
zsets_compaction.L0.RnGB: 0.000000
zsets_compaction.L0.Rnp1GB: 0.000000
zsets_compaction.L0.Score: 0.250000
zsets_compaction.L0.SizeBytes: 35608.000000
zsets_compaction.L0.WblobGB: 0.000000
zsets_compaction.L0.WnewGB: 0.000000
zsets_compaction.L0.WriteAmp: 0.000000
zsets_compaction.L0.WriteGB: 0.000000
zsets_compaction.L0.WriteMBps: 0.000000
zsets_compaction.Sum.AvgSec: 0.000000
zsets_compaction.Sum.CompCount: 0.000000
zsets_compaction.Sum.CompMergeCPU: 0.000000
zsets_compaction.Sum.CompSec: 0.000000
zsets_compaction.Sum.CompactedFiles: 0.000000
zsets_compaction.Sum.KeyDrop: 0.000000
zsets_compaction.Sum.KeyIn: 0.000000
zsets_compaction.Sum.MovedGB: 0.000000
zsets_compaction.Sum.NumFiles: 1.000000
zsets_compaction.Sum.RblobGB: 0.000000
zsets_compaction.Sum.ReadGB: 0.000000
zsets_compaction.Sum.ReadMBps: 0.000000
zsets_compaction.Sum.RnGB: 0.000000
zsets_compaction.Sum.Rnp1GB: 0.000000
zsets_compaction.Sum.Score: 0.000000
zsets_compaction.Sum.SizeBytes: 35608.000000
zsets_compaction.Sum.WblobGB: 0.000000
zsets_compaction.Sum.WnewGB: 0.000000
zsets_compaction.Sum.WriteAmp: 0.000000
zsets_compaction.Sum.WriteGB: 0.000000
zsets_compaction.Sum.WriteMBps: 0.000000
zsets_l0-file-count-limit-delays: 0
zsets_l0-file-count-limit-stops: 0
zsets_memtable-limit-delays: 0
zsets_memtable-limit-stops: 0
zsets_pending-compaction-bytes-delays: 0
zsets_pending-compaction-bytes-stops: 0
zsets_total-delays: 0
zsets_total-stops: 0
`
