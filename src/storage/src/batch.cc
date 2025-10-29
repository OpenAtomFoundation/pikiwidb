// Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include "storage/storage.h"  
#include "storage/batch.h"
#include "src/redis.h"  
#include "glog/logging.h"
#include "binlog.pb.h"  
#include <sys/time.h>

namespace storage {

// ==================== Batch 工厂方法 ====================

std::unique_ptr<Batch> Batch::CreateBatch(Redis* redis) {
  if (redis->GetStorage() && redis->GetStorage()->IsRaftEnabled()) {
    return std::make_unique<BinlogBatch>(
      redis->GetStorage()->GetAppendLogFunction(),
      0,   // db_id
      0,   // slot_idx
      10   // timeout_s
    );
  } else {
    return std::make_unique<RocksBatch>(
      redis->GetDB(),
      redis->GetWriteOptions(),
      redis->GetHandles()
    );
  }
}

// ==================== RocksBatch 实现 ====================

RocksBatch::RocksBatch(rocksdb::DB* db,
                       const rocksdb::WriteOptions& options,
                       const std::vector<rocksdb::ColumnFamilyHandle*>& handles)
    : db_(db), options_(options), handles_(handles) {
}

void RocksBatch::Put(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key, const rocksdb::Slice& value) {
  if (handles_.empty() || cf_idx >= handles_.size()) {
    batch_.Put(key, value);
  } else {
    batch_.Put(handles_[cf_idx], key, value);
  }
  count_++;
}

void RocksBatch::Delete(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key) {
  if (handles_.empty() || cf_idx >= handles_.size()) {
    batch_.Delete(key);
  } else {
    batch_.Delete(handles_[cf_idx], key);
  }
  count_++;
}

rocksdb::Status RocksBatch::Commit() {
  return db_->Write(options_, &batch_);
}

// ==================== BinlogBatch 实现 ====================

BinlogBatch::BinlogBatch(AppendLogFunction func, uint32_t db_id, uint32_t slot_idx, uint32_t timeout_s)
    : append_log_func_(std::move(func)),
      binlog_(std::make_unique<::pikiwidb::Binlog>()),
      timeout_seconds_(timeout_s) {
  binlog_->set_db_id(db_id);
  binlog_->set_slot_idx(slot_idx);
}

BinlogBatch::~BinlogBatch() = default;

void BinlogBatch::Put(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key, const rocksdb::Slice& value) {
  auto* entry = binlog_->add_entries();
  entry->set_cf_idx(cf_idx);
  entry->set_op_type(::pikiwidb::OperateType::kPut);
  entry->set_key(key.data(), key.size());
  entry->set_value(value.data(), value.size());
  count_++;
}

void BinlogBatch::Delete(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key) {
  auto* entry = binlog_->add_entries();
  entry->set_cf_idx(cf_idx);
  entry->set_op_type(::pikiwidb::OperateType::kDelete);
  entry->set_key(key.data(), key.size());
  count_++;
}

rocksdb::Status BinlogBatch::Commit() {
  if (count_ == 0) {
    return rocksdb::Status::OK();
  }
  
  std::promise<rocksdb::Status> promise;
  auto future = promise.get_future();
  
  append_log_func_(*binlog_, std::move(promise));
  
  auto status = future.wait_for(std::chrono::seconds(timeout_seconds_));
  if (status == std::future_status::timeout) {
    LOG(ERROR) << "Raft apply timeout after " << timeout_seconds_ << " seconds";
    return rocksdb::Status::TimedOut("Wait for Raft apply timeout");
  }
  
  return future.get();
}

} // namespace storage

