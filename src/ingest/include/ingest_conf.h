#ifndef PIKA_INGEST_CONF_H_
#define PIKA_INGEST_CONF_H_

#include <atomic>
#include <map>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/utilities/options_util.h"

#include "pstd/include/base_conf.h"

// RocksDB 大规模导入配置类
class IngestConf : public pstd::BaseConf {
 public:
  explicit IngestConf(const std::string& path) : pstd::BaseConf(path), conf_path_(path) {}
  ~IngestConf() override = default;

  // 载入配置（导入期/恢复期两套）
  int Load() {
    int ret = LoadConf();
    if (ret) return ret;

    // 导入期
    GetConfInt("ingest.aggr.max_background_jobs", &aggr_max_background_jobs_);
    if (aggr_max_background_jobs_ <= 0) aggr_max_background_jobs_ = 16;

    GetConfInt("ingest.aggr.max_subcompactions", &aggr_max_subcompactions_);
    if (aggr_max_subcompactions_ <= 0) aggr_max_subcompactions_ = 8;

    GetConfInt("ingest.aggr.level0-file-num-compaction-trigger", &aggr_l0_compact_trigger_);
    if (aggr_l0_compact_trigger_ <= 0) aggr_l0_compact_trigger_ = 1000;

    GetConfInt("ingest.aggr.level0-slowdown-writes-trigger", &aggr_l0_slowdown_trigger_);
    if (aggr_l0_slowdown_trigger_ <= 0) aggr_l0_slowdown_trigger_ = 2000;

    GetConfInt("ingest.aggr.level0-stop-writes-trigger", &aggr_l0_stop_trigger_);
    if (aggr_l0_stop_trigger_ <= 0) aggr_l0_stop_trigger_ = 4000;

    GetConfInt64("ingest.aggr.soft-pending-compaction-bytes-limit", &aggr_soft_pcb_);
    if (aggr_soft_pcb_ <= 0) aggr_soft_pcb_ = 512ll * 1024 * 1024 * 1024;  // 512GB

    GetConfInt64("ingest.aggr.hard-pending-compaction-bytes-limit", &aggr_hard_pcb_);
    if (aggr_hard_pcb_ <= 0) aggr_hard_pcb_ = 1024ll * 1024 * 1024 * 1024;  // 1TB

    GetConfStr("ingest.aggr.disable-auto-compactions", &aggr_disable_auto_compactions_str_);
    if (aggr_disable_auto_compactions_str_.empty()) aggr_disable_auto_compactions_str_ = "true";

    GetConfInt64("ingest.aggr.max_total_wal_size", &aggr_max_total_wal_size_);
    if (aggr_max_total_wal_size_ <= 0) aggr_max_total_wal_size_ = 1073741824;  // 默认 1GB

    // 恢复期
    GetConfInt("ingest.restore.max_background_jobs", &restore_max_background_jobs_);
    if (restore_max_background_jobs_ <= 0) restore_max_background_jobs_ = 4;

    GetConfInt("ingest.restore.max_subcompactions", &restore_max_subcompactions_);
    if (restore_max_subcompactions_ <= 0) restore_max_subcompactions_ = 2;

    GetConfInt("ingest.restore.level0-file-num-compaction-trigger", &restore_l0_compact_trigger_);
    if (restore_l0_compact_trigger_ <= 0) restore_l0_compact_trigger_ = 10;

    GetConfInt("ingest.restore.level0-slowdown-writes-trigger", &restore_l0_slowdown_trigger_);
    if (restore_l0_slowdown_trigger_ <= 0) restore_l0_slowdown_trigger_ = 60;

    GetConfInt("ingest.restore.level0-stop-writes-trigger", &restore_l0_stop_trigger_);
    if (restore_l0_stop_trigger_ <= 0) restore_l0_stop_trigger_ = 120;

    GetConfInt64("ingest.restore.soft-pending-compaction-bytes-limit", &restore_soft_pcb_);
    if (restore_soft_pcb_ <= 0) restore_soft_pcb_ = 64ll * 1024 * 1024 * 1024;  // 64GB

    GetConfInt64("ingest.restore.hard-pending-compaction-bytes-limit", &restore_hard_pcb_);
    if (restore_hard_pcb_ <= 0) restore_hard_pcb_ = 128ll * 1024 * 1024 * 1024;  // 128GB

    GetConfStr("ingest.restore.disable-auto-compactions", &restore_disable_auto_compactions_str_);
    if (restore_disable_auto_compactions_str_.empty()) restore_disable_auto_compactions_str_ = "false";

    GetConfInt64("ingest.restore.max_total_wal_size", &restore_max_total_wal_size_);
    if (restore_max_total_wal_size_ <= 0) restore_max_total_wal_size_ = 268435456;  // 默认 256MB

    GetConfStr("ingest.options.move-files", &opt_move_files_);
    if (opt_move_files_.empty()) opt_move_files_ = "true";

    GetConfStr("ingest.options.verify-checksums-before-ingest", &opt_verify_);
    if (opt_verify_.empty()) opt_verify_ = "true";

    GetConfStr("ingest.options.snapshot-consistency", &opt_snapshot_consistency_);
    if (opt_snapshot_consistency_.empty()) opt_snapshot_consistency_ = "true";

    GetConfStr("ingest.options.allow-blocking-flush", &opt_allow_blocking_flush_);
    if (opt_allow_blocking_flush_.empty()) opt_allow_blocking_flush_ = "true";

    GetConfStr("ingest.options.ingest-behind", &opt_ingest_behind_);
    if (opt_ingest_behind_.empty()) opt_ingest_behind_ = "false";

    GetConfStr("ingest.options.write-global-seqno", &opt_write_global_seqno_);
    if (opt_write_global_seqno_.empty()) opt_write_global_seqno_ = "true";

    GetConfStr("ingest.options.allow-global-seqno", &opt_allow_global_seqno_);
    if (opt_allow_global_seqno_.empty()) opt_allow_global_seqno_ = "true";

    return 0;
  }

  rocksdb::Status ApplyAggressiveOptions(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* cf) {
    if (!db || !cf) return rocksdb::Status::InvalidArgument("db/cf nullptr");

    // 先关自动压缩
    {
      std::unordered_map<std::string, std::string> m;
      m["disable_auto_compactions"] = aggr_disable_auto_compactions_str_;
      auto s = db->SetOptions(cf, m);
      if (!s.ok()) return s;
    }

    {
      std::unordered_map<std::string, std::string> m;
      m["level0_file_num_compaction_trigger"] = std::to_string(aggr_l0_compact_trigger_);
      m["level0_slowdown_writes_trigger"] = std::to_string(aggr_l0_slowdown_trigger_);
      m["level0_stop_writes_trigger"] = std::to_string(aggr_l0_stop_trigger_);
      m["soft_pending_compaction_bytes_limit"] = std::to_string(aggr_soft_pcb_);
      m["hard_pending_compaction_bytes_limit"] = std::to_string(aggr_hard_pcb_);
      auto s = db->SetOptions(cf, m);
      if (!s.ok()) return s;
    }

    {
      std::unordered_map<std::string, std::string> m;
      m["max_background_jobs"] = std::to_string(aggr_max_background_jobs_);
      m["max_subcompactions"] = std::to_string(aggr_max_subcompactions_);
      m["max_total_wal_size"] = std::to_string(aggr_max_total_wal_size_);
      auto s = db->SetDBOptions(m);
      if (!s.ok()) return s;
    }

    return rocksdb::Status::OK();
  }

  rocksdb::Status ApplyRestoreOptions(rocksdb::DB* db, rocksdb::ColumnFamilyHandle* cf) {
    if (!db || !cf) return rocksdb::Status::InvalidArgument("db/cf nullptr");

    {
      std::unordered_map<std::string, std::string> m;
      m["disable_auto_compactions"] = restore_disable_auto_compactions_str_;
      m["level0_file_num_compaction_trigger"] = std::to_string(restore_l0_compact_trigger_);
      m["level0_slowdown_writes_trigger"] = std::to_string(restore_l0_slowdown_trigger_);
      m["level0_stop_writes_trigger"] = std::to_string(restore_l0_stop_trigger_);
      m["soft_pending_compaction_bytes_limit"] = std::to_string(restore_soft_pcb_);
      m["hard_pending_compaction_bytes_limit"] = std::to_string(restore_hard_pcb_);
      auto s = db->SetOptions(cf, m);
      if (!s.ok()) return s;
    }

    {
      std::unordered_map<std::string, std::string> m;
      m["max_background_jobs"] = std::to_string(restore_max_background_jobs_);
      m["max_subcompactions"] = std::to_string(restore_max_subcompactions_);
      m["max_total_wal_size"] = std::to_string(restore_max_total_wal_size_);
      auto s = db->SetDBOptions(m);
      if (!s.ok()) return s;
    }

    return rocksdb::Status::OK();
  }

  rocksdb::IngestExternalFileOptions MakeIngestOptions() const {
    rocksdb::IngestExternalFileOptions opt;
    opt.move_files = (opt_move_files_ == "true");
    opt.verify_checksums_before_ingest = (opt_verify_ == "true");
    opt.snapshot_consistency = (opt_snapshot_consistency_ == "true");
    opt.allow_blocking_flush = (opt_allow_blocking_flush_ == "true");
    opt.ingest_behind = (opt_ingest_behind_ == "true");
    opt.write_global_seqno = (opt_write_global_seqno_ == "true"); 
    opt.allow_global_seqno = (opt_allow_global_seqno_ == "true");
    return opt;
  }

  std::string conf_path() const {
    std::shared_lock lk(rwlock_);
    return conf_path_;
  }

  int aggr_max_background_jobs() const { return aggr_max_background_jobs_; }
  int aggr_max_subcompactions() const { return aggr_max_subcompactions_; }
  int aggr_l0_compact_trigger() const { return aggr_l0_compact_trigger_; }
  int aggr_l0_slowdown_trigger() const { return aggr_l0_slowdown_trigger_; }
  int aggr_l0_stop_trigger() const { return aggr_l0_stop_trigger_; }
  int64_t aggr_soft_pcb() const { return aggr_soft_pcb_; }
  int64_t aggr_hard_pcb() const { return aggr_hard_pcb_; }
  std::string aggr_disable_auto_compactions_str() const { return aggr_disable_auto_compactions_str_; }
  int64_t aggr_max_total_wal_size() const { return aggr_max_total_wal_size_; }

  int restore_max_background_jobs() const { return restore_max_background_jobs_; }
  int restore_max_subcompactions() const { return restore_max_subcompactions_; }
  int restore_l0_compact_trigger() const { return restore_l0_compact_trigger_; }
  int restore_l0_slowdown_trigger() const { return restore_l0_slowdown_trigger_; }
  int restore_l0_stop_trigger() const { return restore_l0_stop_trigger_; }
  int64_t restore_soft_pcb() const { return restore_soft_pcb_; }
  int64_t restore_hard_pcb() const { return restore_hard_pcb_; }
  std::string restore_disable_auto_compactions_str() const { return restore_disable_auto_compactions_str_; }
  int64_t restore_max_total_wal_size() const { return restore_max_total_wal_size_; }

  void SetAggrMaxBackgroundJobs(int v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.max_background_jobs", std::to_string(v));
    aggr_max_background_jobs_ = v;
  }
  void SetAggrMaxSubcompactions(int v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.max_subcompactions", std::to_string(v));
    aggr_max_subcompactions_ = v;
  }
  void SetAggrL0CompactTrigger(int v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.level0-file-num-compaction-trigger", std::to_string(v));
    aggr_l0_compact_trigger_ = v;
  }
  void SetAggrL0SlowdownTrigger(int v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.level0-slowdown-writes-trigger", std::to_string(v));
    aggr_l0_slowdown_trigger_ = v;
  }
  void SetAggrL0StopTrigger(int v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.level0-stop-writes-trigger", std::to_string(v));
    aggr_l0_stop_trigger_ = v;
  }
  void SetAggrSoftPCB(int64_t v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.soft-pending-compaction-bytes-limit", std::to_string(v));
    aggr_soft_pcb_ = v;
  }
  void SetAggrHardPCB(int64_t v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.hard-pending-compaction-bytes-limit", std::to_string(v));
    aggr_hard_pcb_ = v;
  }
  void SetAggrMaxTotalWalSize(int64_t v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.max_total_wal_size", std::to_string(v));
    aggr_max_total_wal_size_ = v;
  }
  void SetAggrDisableAutoCompactionsStr(const std::string& v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.aggr.disable-auto-compactions", v);
    aggr_disable_auto_compactions_str_ = v;
  }

  void SetRestoreMaxBackgroundJobs(int value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.max_background_jobs", std::to_string(value));
    restore_max_background_jobs_ = value;
  }
  void SetRestoreMaxSubcompactions(int value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.max_subcompactions", std::to_string(value));
    restore_max_subcompactions_ = value;
  }
  void SetRestoreL0CompactTrigger(int value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.level0-file-num-compaction-trigger", std::to_string(value));
    restore_l0_compact_trigger_ = value;
  }
  void SetRestoreL0SlowdownTrigger(int value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.level0-slowdown-writes-trigger", std::to_string(value));
    restore_l0_slowdown_trigger_ = value;
  }
  void SetRestoreL0StopTrigger(int value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.level0-stop-writes-trigger", std::to_string(value));
    restore_l0_stop_trigger_ = value;
  }
  void SetRestoreSoftPCB(int64_t value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.soft-pending-compaction-bytes-limit", std::to_string(value));
    restore_soft_pcb_ = value;
  }
  void SetRestoreHardPCB(int64_t value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.hard-pending-compaction-bytes-limit", std::to_string(value));
    restore_hard_pcb_ = value;
  }
  void SetRestoreMaxTotalWalSize(int64_t v) {
    std::lock_guard lk(rwlock_);
    TryPushDiffCommands("ingest.restore.max_total_wal_size", std::to_string(v));
    restore_max_total_wal_size_ = v;
  }
  void SetRestoreDisableAutoCompactionsStr(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.restore.disable-auto-compactions", value);
    restore_disable_auto_compactions_str_ = value;
  }

  void SetOptMoveFiles(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.move-files", value);
    opt_move_files_ = value;
  }
  void SetOptVerify(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.verify-checksums-before-ingest", value);
    opt_verify_ = value;
  }
  void SetOptSnapshotConsistency(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.snapshot-consistency", value);
    opt_snapshot_consistency_ = value;
  }
  void SetOptAllowBlockingFlush(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.allow-blocking-flush", value);
    opt_allow_blocking_flush_ = value;
  }
  void SetOptIngestBehind(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.ingest-behind", value);
    opt_ingest_behind_ = value;
  }
  void SetOptWriteGlobalSeqno(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.write-global-seqno", value);
    opt_write_global_seqno_ = value;
  }
   void SetOptAllowGlobalSeqno(const std::string& value) {
    std::lock_guard<std::shared_mutex> lk(rwlock_);
    TryPushDiffCommands("ingest.options.allow-global-seqno", value);
    opt_allow_global_seqno_ = value;
  }
  int ConfigRewrite() {
    int err = 0;
    for (const auto& kv : diff_commands_) {
      if (!SetConfStr(kv.first, kv.second)) {  
        err = -1;
      }
    }
    diff_commands_.clear();
    return err;
  }

 private:
  void TryPushDiffCommands(const std::string& key, const std::string& value) { diff_commands_[key] = value; }

  std::string conf_path_;
  mutable std::shared_mutex rwlock_;
  std::map<std::string, std::string> diff_commands_;

  int aggr_max_background_jobs_ = 16;
  int aggr_max_subcompactions_ = 8;
  int aggr_l0_compact_trigger_ = 1000;
  int aggr_l0_slowdown_trigger_ = 2000;
  int aggr_l0_stop_trigger_ = 4000;
  int64_t aggr_soft_pcb_ = 512ll * 1024 * 1024 * 1024;   // 512GB
  int64_t aggr_hard_pcb_ = 1024ll * 1024 * 1024 * 1024;  // 1TB
  int64_t aggr_max_total_wal_size_ = 1073741824;         // 1GB 默认值
  std::string aggr_disable_auto_compactions_str_ = "true";

  int aggr_dummy_pad_ = 0; 

  int restore_max_background_jobs_ = 4;
  int restore_max_subcompactions_ = 2;
  int restore_l0_compact_trigger_ = 10;
  int restore_l0_slowdown_trigger_ = 60;
  int restore_l0_stop_trigger_ = 120;
  int64_t restore_soft_pcb_ = 64ll * 1024 * 1024 * 1024;   // 64GB
  int64_t restore_hard_pcb_ = 128ll * 1024 * 1024 * 1024;  // 128GB
  int64_t restore_max_total_wal_size_ = 268435456;         // 256MB 默认值
  std::string restore_disable_auto_compactions_str_ = "false";

  std::string opt_move_files_ = "true";
  std::string opt_verify_ = "true";
  std::string opt_snapshot_consistency_ = "true";
  std::string opt_allow_blocking_flush_ = "true";
  std::string opt_ingest_behind_ = "false";
  std::string opt_write_global_seqno_ = "true";
  std::string opt_allow_global_seqno_ = "true";
};

#endif  // PIKA_INGEST_CONF_H_
