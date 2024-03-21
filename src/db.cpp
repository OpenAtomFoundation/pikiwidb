//
// Created by dingxiaoshuai on 2024/3/18.
//

#include "db.h"
#include "config.h"

extern pikiwidb::PConfig g_config;

namespace pikiwidb {

struct CheckPointInfo;

DB::DB(int db_index, const std::string& db_path, const std::string& dump_parent_path)
    : db_index_(db_index),
      db_path_(db_path + std::to_string(db_index_) + '/'),
      dump_parent_path_(dump_parent_path),
      dump_path_(dump_parent_path + std::to_string(db_index_)) {
  storage::StorageOptions storage_options;
  storage_options.options.create_if_missing = true;
  storage_options.db_instance_num = g_config.db_instance_num;
  storage_options.db_id = db_index_;

  // options for CF
  storage_options.options.ttl = g_config.rocksdb_ttl_second;
  storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;
  storage_ = std::make_unique<storage::Storage>();
  storage_->Open(storage_options, db_path_);

  checkpoint_manager_ = std::make_unique<CheckpointManager>();
  // ./dump/1
  checkpoint_manager_->Init(g_config.db_instance_num, dump_path_, this);

  opened_ = true;
  INFO("Open DB{} success!", db_index_);
}

void DB::DoBgSave(CheckPointInfo& checkpoint_info, const std::string& path, int i) {
  // 1) always hold storage's sharedLock
  std::shared_lock sharedLock(storage_mutex_);

  // 2）Create the storage's checkpoint 。
  auto status = storage_->CreateCheckpoint(path, i);

  // 3) write the status
  if (status.ok()) {
    checkpoint_info.last_checkpoint_success = true;
    checkpoint_info.last_checkpoint_time = time(nullptr);
    // others
  }

  // 用于测试是否能够同步等待。
  sleep(2);
}

void DB::CreateCheckpoint() {
  {
    std::lock_guard lock(checkpoint_mutex_);
    if (checkpoint_in_process) {
      return;
    }
    checkpoint_in_process = true;
  }
  checkpoint_manager_->CreateCheckpoint();
}

void DB::FinishCheckpointDone(bool sync) { checkpoint_manager_->Finish(sync); }
}  // namespace pikiwidb
