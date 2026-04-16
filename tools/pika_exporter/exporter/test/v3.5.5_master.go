package test

var V355MasterInfo = `# Server
pika_version:3.5.5
pika_git_sha:29a7629bc97c237531f12689b92cb59ee73bdef7
pika_build_compile_date: 2025-07-30 10:25:07
os:Linux 4.19.91-27.2.an8.x86_64 x86_64
arch_bits:64
process_id:3483206
tcp_port:5436
thread_num:4
sync_thread_num:2
sync_binlog_thread_num:1
uptime_in_seconds:11511104
uptime_in_days:134
config_file:/data02/pika5436/pika5436.conf
server_id:1
run_id:55070bbe17d5ac6c5310b86f367efcbf26e23f87

# Data
db_size:22812537818
db_size_human:21755M
log_size:991009996
log_size_human:945M
compression:snappy
used_memory:521969468
used_memory_human:497M
db_memtable_usage:33579008
db_tablereader_usage:488390460
db_fatal:0
db_fatal_msg:nullptr

# Clients
connected_clients:1

# Stats
total_connections_received:118661
instantaneous_ops_per_sec:1
total_commands_processed:66245
total_net_input_bytes:45958842766
total_net_output_bytes:119880559018
total_net_repl_input_bytes:45548052381
total_net_repl_output_bytes:68472471039
instantaneous_input_kbps:0.474609
instantaneous_output_kbps:22.9424
instantaneous_input_repl_kbps:0.290039
instantaneous_output_repl_kbps:0.176758
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
SET:238193967
SADD:35933
INFO:2663740
GET:39816
SLAVEOF:15
ZADD:11003
PING:15
AUTH:2849789
MONITOR:16
BGSAVE:25
HMSET:102630
SCAN:19
RPUSH:16338
CONFIG:4984234

# Commandstats
slaveof:calls=15, usec=3.08, usec_per_call=0.21
bgsave:calls=25, usec=1.88, usec_per_call=0.08
info:calls=2663739, usec=3914175.92, usec_per_call=1.47
monitor:calls=16, usec=0.37, usec_per_call=0.02
get:calls=39816, usec=1158.24, usec_per_call=0.03
ping:calls=15, usec=0.26, usec_per_call=0.02
auth:calls=2849789, usec=90852.64, usec_per_call=0.03
scan:calls=19, usec=3.21, usec_per_call=0.17
config:calls=4984234, usec=113595.00, usec_per_call=0.02
set:calls=193039, usec=15532.41, usec_per_call=0.08

# Cache
cache_status:Disable

# CPU
used_cpu_sys:4282.90
used_cpu_user:9610.00
used_cpu_sys_children:0.00
used_cpu_user_children:0.00

# Replication(MASTER)
role:master
ReplicationID:40174f033ff85653ac7fd3ec4d9de8f33c2e21e95bd1262970
connected_slaves:2
slave0:ip=10.242.36.21,port=5436,conn_fd=1078,lag=(db0:0)
slave1:ip=10.242.36.49,port=5436,conn_fd=66,lag=(db0:0)
is_eligible_for_master_election:true
db0:binlog_offset=368 42825120,safety_purge=write2file358
slave_repl_offset:38630421920

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
strings_estimate_num_keys:239070333
strings_estimate_table_readers_mem:472254516
strings_num_snapshots:0
strings_num_live_versions:1
strings_current_super_version_number:1627
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
strings_compaction.L0.AvgSec: 0.577369
strings_compaction.L0.CompCount: 132.000000
strings_compaction.L0.CompMergeCPU: 75.306574
strings_compaction.L0.CompSec: 76.212661
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
strings_compaction.L0.WnewGB: 20.574955
strings_compaction.L0.WriteAmp: 1.000000
strings_compaction.L0.WriteGB: 20.574955
strings_compaction.L0.WriteMBps: 276.446897
strings_compaction.L4.AvgSec: 2.809312
strings_compaction.L4.CompCount: 59.000000
strings_compaction.L4.CompMergeCPU: 153.335163
strings_compaction.L4.CompSec: 165.749412
strings_compaction.L4.CompactedFiles: 0.000000
strings_compaction.L4.KeyDrop: 85.000000
strings_compaction.L4.KeyIn: 330909613.000000
strings_compaction.L4.MovedGB: 0.000000
strings_compaction.L4.NumFiles: 14.000000
strings_compaction.L4.RblobGB: 0.000000
strings_compaction.L4.ReadGB: 25.316844
strings_compaction.L4.ReadMBps: 156.407483
strings_compaction.L4.RnGB: 17.299404
strings_compaction.L4.Rnp1GB: 8.017440
strings_compaction.L4.Score: 0.927166
strings_compaction.L4.SizeBytes: 248884307.000000
strings_compaction.L4.WblobGB: 0.000000
strings_compaction.L4.WnewGB: 17.299660
strings_compaction.L4.WriteAmp: 1.463467
strings_compaction.L4.WriteGB: 25.317100
strings_compaction.L4.WriteMBps: 156.409060
strings_compaction.L5.AvgSec: 0.288949
strings_compaction.L5.CompCount: 489.000000
strings_compaction.L5.CompMergeCPU: 137.468382
strings_compaction.L5.CompSec: 141.295883
strings_compaction.L5.CompactedFiles: 0.000000
strings_compaction.L5.KeyDrop: 9.000000
strings_compaction.L5.KeyIn: 238593642.000000
strings_compaction.L5.MovedGB: 1.047475
strings_compaction.L5.NumFiles: 89.000000
strings_compaction.L5.RblobGB: 0.000000
strings_compaction.L5.ReadGB: 18.941386
strings_compaction.L5.ReadMBps: 137.272077
strings_compaction.L5.RnGB: 18.815442
strings_compaction.L5.Rnp1GB: 0.125945
strings_compaction.L5.Score: 0.989415
strings_compaction.L5.SizeBytes: 1928919001.000000
strings_compaction.L5.WblobGB: 0.000000
strings_compaction.L5.WnewGB: 18.669424
strings_compaction.L5.WriteAmp: 0.998933
strings_compaction.L5.WriteGB: 18.795369
strings_compaction.L5.WriteMBps: 136.213859
strings_compaction.L6.AvgSec: 0.278018
strings_compaction.L6.CompCount: 24.000000
strings_compaction.L6.CompMergeCPU: 6.106510
strings_compaction.L6.CompSec: 6.672425
strings_compaction.L6.CompactedFiles: 0.000000
strings_compaction.L6.KeyDrop: 0.000000
strings_compaction.L6.KeyIn: 5104306.000000
strings_compaction.L6.MovedGB: 15.653928
strings_compaction.L6.NumFiles: 877.000000
strings_compaction.L6.RblobGB: 0.000000
strings_compaction.L6.ReadGB: 2.772516
strings_compaction.L6.ReadMBps: 425.490854
strings_compaction.L6.RnGB: 2.747027
strings_compaction.L6.Rnp1GB: 0.025489
strings_compaction.L6.Score: 0.000000
strings_compaction.L6.SizeBytes: 19495543195.000000
strings_compaction.L6.WblobGB: 0.000000
strings_compaction.L6.WnewGB: 2.502712
strings_compaction.L6.WriteAmp: 0.920341
strings_compaction.L6.WriteGB: 2.528201
strings_compaction.L6.WriteMBps: 387.996511
strings_compaction.Sum.AvgSec: 0.553878
strings_compaction.Sum.CompCount: 704.000000
strings_compaction.Sum.CompMergeCPU: 372.216629
strings_compaction.Sum.CompSec: 389.930381
strings_compaction.Sum.CompactedFiles: 0.000000
strings_compaction.Sum.KeyDrop: 94.000000
strings_compaction.Sum.KeyIn: 574607561.000000
strings_compaction.Sum.MovedGB: 16.701402
strings_compaction.Sum.NumFiles: 981.000000
strings_compaction.Sum.RblobGB: 0.000000
strings_compaction.Sum.ReadGB: 47.030746
strings_compaction.Sum.ReadMBps: 123.507904
strings_compaction.Sum.RnGB: 38.861873
strings_compaction.Sum.Rnp1GB: 8.168874
strings_compaction.Sum.Score: 0.000000
strings_compaction.Sum.SizeBytes: 21673347794.000000
strings_compaction.Sum.WblobGB: 0.000000
strings_compaction.Sum.WnewGB: 59.046751
strings_compaction.Sum.WriteAmp: 3.266866
strings_compaction.Sum.WriteGB: 67.215624
strings_compaction.Sum.WriteMBps: 176.515610
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
hashes_current_super_version_number:6
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
hashes_compaction.L0.AvgSec: 0.030032
hashes_compaction.L0.CompCount: 1.000000
hashes_compaction.L0.CompMergeCPU: 0.029547
hashes_compaction.L0.CompSec: 0.030032
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
hashes_compaction.L0.WnewGB: 0.003229
hashes_compaction.L0.WriteAmp: 1.000000
hashes_compaction.L0.WriteGB: 0.003229
hashes_compaction.L0.WriteMBps: 110.102943
hashes_compaction.Sum.AvgSec: 0.030032
hashes_compaction.Sum.CompCount: 1.000000
hashes_compaction.Sum.CompMergeCPU: 0.029547
hashes_compaction.Sum.CompSec: 0.030032
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
hashes_compaction.Sum.WnewGB: 0.003229
hashes_compaction.Sum.WriteAmp: 1.000000
hashes_compaction.Sum.WriteGB: 0.003229
hashes_compaction.Sum.WriteMBps: 110.102943
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
lists_current_super_version_number:6
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
lists_compaction.L0.AvgSec: 0.006108
lists_compaction.L0.CompCount: 1.000000
lists_compaction.L0.CompMergeCPU: 0.005811
lists_compaction.L0.CompSec: 0.006108
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
lists_compaction.L0.WnewGB: 0.000785
lists_compaction.L0.WriteAmp: 1.000000
lists_compaction.L0.WriteGB: 0.000785
lists_compaction.L0.WriteMBps: 131.514951
lists_compaction.Sum.AvgSec: 0.006108
lists_compaction.Sum.CompCount: 1.000000
lists_compaction.Sum.CompMergeCPU: 0.005811
lists_compaction.Sum.CompSec: 0.006108
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
lists_compaction.Sum.WnewGB: 0.000785
lists_compaction.Sum.WriteAmp: 1.000000
lists_compaction.Sum.WriteGB: 0.000785
lists_compaction.Sum.WriteMBps: 131.514951
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
sets_current_super_version_number:15
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
sets_compaction.L0.AvgSec: 0.002983
sets_compaction.L0.CompCount: 1.000000
sets_compaction.L0.CompMergeCPU: 0.002872
sets_compaction.L0.CompSec: 0.002983
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
sets_compaction.L0.WnewGB: 0.000065
sets_compaction.L0.WriteAmp: 1.000000
sets_compaction.L0.WriteGB: 0.000065
sets_compaction.L0.WriteMBps: 22.196898
sets_compaction.Sum.AvgSec: 0.002983
sets_compaction.Sum.CompCount: 1.000000
sets_compaction.Sum.CompMergeCPU: 0.002872
sets_compaction.Sum.CompSec: 0.002983
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
sets_compaction.Sum.WnewGB: 0.000065
sets_compaction.Sum.WriteAmp: 1.000000
sets_compaction.Sum.WriteGB: 0.000065
sets_compaction.Sum.WriteMBps: 22.196898
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
zsets_current_super_version_number:9
zsets_estimate_live_data_size:48903360
zsets_total_sst_files_size:48903360
zsets_live_sst_files_size:48903360
zsets_estimate_pending_compaction_bytes:0
zsets_block_cache_capacity:6442450944
zsets_block_cache_usage:288
zsets_block_cache_pinned_usage:288
zsets_num_blob_files:0
zsets_blob_stats:140585469313112
zsets_total_blob_file_size:0
zsets_live_blob_file_size:0
zsets_cf-l0-file-count-limit-delays-with-ongoing-compaction: 0
zsets_cf-l0-file-count-limit-stops-with-ongoing-compaction: 0
zsets_compaction.L0.AvgSec: 0.001080
zsets_compaction.L0.CompCount: 1.000000
zsets_compaction.L0.CompMergeCPU: 0.001001
zsets_compaction.L0.CompSec: 0.001080
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
zsets_compaction.L0.WnewGB: 0.000033
zsets_compaction.L0.WriteAmp: 1.000000
zsets_compaction.L0.WriteGB: 0.000033
zsets_compaction.L0.WriteMBps: 31.413908
zsets_compaction.Sum.AvgSec: 0.001080
zsets_compaction.Sum.CompCount: 1.000000
zsets_compaction.Sum.CompMergeCPU: 0.001001
zsets_compaction.Sum.CompSec: 0.001080
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
zsets_compaction.Sum.WnewGB: 0.000033
zsets_compaction.Sum.WriteAmp: 1.000000
zsets_compaction.Sum.WriteGB: 0.000033
zsets_compaction.Sum.WriteMBps: 31.413908
zsets_l0-file-count-limit-delays: 0
zsets_l0-file-count-limit-stops: 0
zsets_memtable-limit-delays: 0
zsets_memtable-limit-stops: 0
zsets_pending-compaction-bytes-delays: 0
zsets_pending-compaction-bytes-stops: 0
zsets_total-delays: 0
zsets_total-stops: 0
`
