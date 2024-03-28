/*
 * Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "db.h"
#include "config.h"
#include "praft/praft.h"
#include "pstd/log.h"

extern pikiwidb::PConfig g_config;

namespace pikiwidb {

struct CheckPointInfo;

DB::DB(int db_index, const std::string& db_path)
    : db_index_(db_index), db_path_(db_path + std::to_string(db_index_) + '/') {
  storage::StorageOptions storage_options;
  storage_options.options.create_if_missing = true;
  storage_options.db_instance_num = g_config.db_instance_num;
  storage_options.db_id = db_index_;

  // options for CF
  storage_options.options.ttl = g_config.rocksdb_ttl_second;
  storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;
  if (g_config.use_raft) {
    storage_options.append_log_function = [&r = PRAFT](const Binlog& log, std::promise<rocksdb::Status>&& promise) {
      r.AppendLog(log, std::move(promise));
    };
  }
  storage_ = std::make_unique<storage::Storage>();

  if (auto s = storage_->Open(storage_options, db_path_); !s.ok()) {
    ERROR("Storage open failed! {}", s.ToString());
    abort();
  }
  opened_ = true;
  INFO("Open DB{} success!", db_index_);
}

void DB::DoBgSave(CheckpointInfo& checkpoint_info, const std::string& path, int i) {
  // 1) always hold storage's sharedLock
  std::shared_lock sharedLock(storage_mutex_);

  // 2）Create the storage's checkpoint 。
  auto status = storage_->CreateCheckpoint(path, i);

  // 3) write the status
  checkpoint_info.checkpoint_in_process = false;
}

void DB::CreateCheckpoint(const std::string& path) { checkpoint_manager_->CreateCheckpoint(path); }

void DB::WaitForCheckpointDone() { checkpoint_manager_->WaitForCheckpointDone(); }

}  // namespace pikiwidb
