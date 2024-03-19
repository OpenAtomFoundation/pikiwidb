//
// Created by dingxiaoshuai on 2024/3/18.
//

#include "db.h"
#include "config.h"

extern pikiwidb::PConfig g_config;

namespace pikiwidb {

DB::DB(int db_id, const std::string &db_path) : db_id_(db_id), db_path_(db_path + std::to_string(db_id) + '/') {
  storage::StorageOptions storage_options;
  storage_options.options.create_if_missing = true;
  storage_options.db_instance_num = g_config.db_instance_num;
  storage_options.db_id = db_id;

  // options for CF
  storage_options.options.ttl = g_config.rocksdb_ttl_second;
  storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;
  storage_ = std::make_unique<storage::Storage>();
  if (auto s = storage_->Open(storage_options, db_path_); !s.ok()) {
    ERROR("Storage open failed! {}", s.ToString());
    abort();
  }
  opened_ = true;
  INFO("Open DB{} success!", db_id);
}
}  // namespace pikiwidb
