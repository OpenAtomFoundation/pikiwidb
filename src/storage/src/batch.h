/*
 * Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>

#include "rocksdb/db.h"

#include "binlog.pb.h"
#include "src/redis.h"
#include "storage/storage.h"
#include "storage/storage_define.h"

namespace storage {

class Batch {
 public:
  virtual ~Batch() = default;

  virtual void Put(ColumnFamilyIndex cf_idx, const Slice& key, const Slice& val) = 0;
  virtual void Delete(ColumnFamilyIndex cf_idx, const Slice& key) = 0;
  virtual auto Commit() -> Status = 0;

  static auto CreateBatch(Redis* redis) -> std::unique_ptr<Batch>;
};

class RocksBatch : public Batch {
 public:
  RocksBatch(rocksdb::DB* db, const rocksdb::WriteOptions& options,
             const std::vector<rocksdb::ColumnFamilyHandle*>& handles)
      : db_(db), options_(options), handles_(handles) {}

  void Put(ColumnFamilyIndex cf_idx, const Slice& key, const Slice& val) override {
    batch_.Put(handles_[cf_idx], key, val);
  }
  void Delete(ColumnFamilyIndex cf_idx, const Slice& key) override { batch_.Delete(handles_[cf_idx], key); }
  auto Commit() -> Status override { return db_->Write(options_, &batch_); }

 private:
  rocksdb::WriteBatch batch_;
  rocksdb::DB* db_;
  const rocksdb::WriteOptions& options_;
  const std::vector<rocksdb::ColumnFamilyHandle*>& handles_;
};

class BinlogBatch : public Batch {
 public:
  BinlogBatch(AppendLogFunction func, int32_t index, uint32_t seconds = 10)
      : func_(std::move(func)), seconds_(seconds) {
    binlog_.set_db_id(0);
    binlog_.set_slot_idx(index);
  }

  void Put(ColumnFamilyIndex cf_idx, const Slice& key, const Slice& value) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(pikiwidb::OperateType::kPut);
    entry->set_key(key.ToString());
    entry->set_value(value.ToString());
  }

  void Delete(ColumnFamilyIndex cf_idx, const Slice& key) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(pikiwidb::OperateType::kDelete);
    entry->set_key(key.ToString());
  }

  Status Commit() override {
    // FIXME(longfar): We should make sure that in non-RAFT mode, the code doesn't run here
    std::promise<Status> promise;
    auto future = promise.get_future();
    func_(binlog_, std::move(promise));
    auto status = future.wait_for(std::chrono::seconds(seconds_));
    if (status == std::future_status::timeout) {
      return Status::Incomplete("Wait for write timeout");
    }
    return future.get();
  }

 private:
  AppendLogFunction func_;
  pikiwidb::Binlog binlog_;
  uint32_t seconds_ = 10;
};

inline auto Batch::CreateBatch(Redis* redis) -> std::unique_ptr<Batch> {
  if (redis->GetAppendLogFunction()) {
    return std::make_unique<BinlogBatch>(redis->GetAppendLogFunction(), redis->GetIndex(), redis->GetRaftTimeout());
  }
  return std::make_unique<RocksBatch>(redis->GetDB(), redis->GetWriteOptions(), redis->GetColumnFamilyHandles());
}

}  // namespace storage
