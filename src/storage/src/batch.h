#pragma once

#include "src/binlog.pb.h"
#include "src/log_queue.h"
#include "src/redis.h"
#include "storage/storage.h"

namespace storage {

class Batch {
 public:
  Batch() = default;

  virtual void Put(int32_t cf_idx, const Slice& key, const Slice& value) = 0;
  virtual void Delete(int32_t cf_idx, const Slice& key) = 0;
  virtual Status Commit() = 0;

  static auto CreateBatch(const Storage* storage, DataType type, bool use_binlog = false) -> std::unique_ptr<Batch>;
};

class RocksBatch : public Batch {
 public:
  RocksBatch(const Storage* storage, DataType type) : redis_(storage->GetRedisByType(type)) {}

  void Put(int32_t cf_idx, const Slice& key, const Slice& value) override {
    if (cf_idx == -1) {
      batch_.Put(key, value);
    } else {
      batch_.Put(redis_->GetColumnFamilyHandle(cf_idx), key, value);
    }
  }

  void Delete(int32_t cf_idx, const Slice& key) override {
    if (cf_idx == -1) {
      batch_.Delete(key);
    } else {
      batch_.Delete(redis_->GetColumnFamilyHandle(cf_idx), key);
    }
  }

  Status Commit() override { return redis_->GetDB()->Write(redis_->GetWriteOptions(), &batch_); }

 private:
  rocksdb::WriteBatch batch_;
  Redis* const redis_;
};

class BinlogBatch : public Batch {
 public:
  BinlogBatch(const Storage* storage, DataType type) : log_queue_(storage->GetLogQueue()) {
    binlog_.set_data_type(static_cast<BinlogDataType>(type));
  }

  void Put(int32_t cf_idx, const Slice& key, const Slice& value) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(OperateType::kPut);
    entry->set_key(key.ToString());
    entry->set_value(value.ToString());
  }

  void Delete(int32_t cf_idx, const Slice& key) override {
    auto entry = binlog_.add_entries();
    entry->set_cf_idx(cf_idx);
    entry->set_op_type(OperateType::kDelete);
    entry->set_key(key.ToString());
  }

  Status Commit() override {
    auto future = log_queue_->Produce(binlog_.SerializeAsString());
    return future.get();
  }

 private:
  Binlog binlog_;
  LogQueue* log_queue_;
};

inline auto Batch::CreateBatch(const Storage* storage, DataType type, bool use_binlog) -> std::unique_ptr<Batch> {
  if (use_binlog) {
    return std::make_unique<BinlogBatch>(storage, type);
  } else {
    return std::make_unique<RocksBatch>(storage, type);
  }
}

}  // namespace storage