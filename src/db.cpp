//
// Created by dingxiaoshuai on 2024/3/18.
//

#include "db.h"
#include "config.h"

extern pikiwidb::PConfig g_config;

namespace pikiwidb {

DB::DB(int db_id, const std::string &db_path, const std::string& checkpoint_sub_path) : db_id_(db_id),
                                                                                        db_path_(db_path + std::to_string(db_id) + '/'),
                                                                                        checkpoint_path_(db_path + checkpoint_sub_path + '/' + std::to_string(db_id)){
  storage::StorageOptions storage_options;
  storage_options.options.create_if_missing = true;
  storage_options.db_instance_num = g_config.db_instance_num;
  storage_options.db_id = db_id;

  // options for CF
  storage_options.options.ttl = g_config.rocksdb_ttl_second;
  storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;
  storage_ = std::make_unique<storage::Storage>();
  storage_->Open(storage_options, db_path_);
  opened_ = true;
  INFO("Open DB{} success!", db_id);
}

void DB::DoBgSave() {
  // 1) always hold storage's sharedLock
  std::shared_lock sharedLock(storage_mutex_);
  {
    std::lock_guard Lock(checkpoint_mutex_);
    if (checkpoint_context_.checkpoint_in_process) {
      return;
    }
    checkpoint_context_.checkpoint_path = checkpoint_path_ + "/";
    INFO("checkpoint_path = {}", checkpoint_context_.checkpoint_path);
    checkpoint_context_.last_checkpoint_success = false;
    // todo 记录当前时间点
    checkpoint_context_.last_checkpoint_time = time(nullptr);
    checkpoint_context_.checkpoint_in_process = true;
  }
  // 2）Create the storage's checkpoint 。
  auto status = storage_->CreateCheckpoint(checkpoint_context_.checkpoint_path);

  // 3)
  std::lock_guard Lock(checkpoint_mutex_);
  if (status.ok()) {
    checkpoint_context_.last_checkpoint_success = true;
  }
  checkpoint_context_.checkpoint_in_process = false;
}
}  // namespace pikiwidb
