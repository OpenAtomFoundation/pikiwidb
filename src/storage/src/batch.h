#pragma once

#include <memory>
#include "rocksdb/db.h"

#include "src/binlog.pb.h"
#include "src/task_queue.h"
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

  static auto CreateBatch(Redis* redis, bool use_binlog = false) -> std::unique_ptr<Batch>;
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
  BinlogBatch(TaskQueue* queue, int32_t index) : task_queue_(queue) { binlog_.set_slot_idx(index); }

  void Put(ColumnFamilyIndex cf_idx, const Slice& key, const Slice& value) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(OperateType::kPut);
    entry->set_key(key.ToString());
    entry->set_value(value.ToString());
  }

  void Delete(ColumnFamilyIndex cf_idx, const Slice& key) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(OperateType::kDelete);
    entry->set_key(key.ToString());
  }

  Status Commit() override {
    std::promise<Status> promise;
    auto future = promise.get_future();
    Task task(binlog_.SerializeAsString(), std::move(promise));
    task_queue_->Produce(std::move(task));
    return future.get();
  }

 private:
  Binlog binlog_;
  TaskQueue* task_queue_;
};

inline auto Batch::CreateBatch(Redis* redis, bool use_binlog) -> std::unique_ptr<Batch> {
  if (use_binlog) {
    return std::make_unique<BinlogBatch>(redis->GetTaskQueue(), redis->GetIndex());
  }
  return std::make_unique<RocksBatch>(redis->GetDB(), redis->GetWriteOptions(), redis->GetColumnFamilyHandles());
}

}  // namespace storage
