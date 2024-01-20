#pragma once

#include "google/protobuf/stubs/callback.h"

#include "src/binlog.pb.h"
#include "src/log_queue.h"
#include "src/redis.h"
#include "storage/storage.h"

namespace storage {

using Closure = ::google::protobuf::Closure;

class RocksClosure : public ::google::protobuf::Closure {
 public:
  explicit RocksClosure(std::promise<rocksdb::Status>&& promise) : promise_(std::move(promise)) {}

  void Run() override { promise_.set_value(result_); }
  void SetStatus(rocksdb::Status&& status) { result_ = std::move(status); }

 private:
  std::promise<rocksdb::Status> promise_;
  rocksdb::Status result_{Status::Aborted("Unknown error")};
};

class Task {
 public:
  Task(std::string&& data, std::promise<Status>&& promise)
      : data_(std::move(data)), done_(std::make_shared<RocksClosure>(std::move(promise))) {}

  std::string data_;
  std::shared_ptr<Closure> done_;
};

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
    std::promise<Status> promise;
    auto future = promise.get_future();
    Task task(binlog_.SerializeAsString(), std::move(promise));
    log_queue_->Produce(std::move(task));
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

class ClosureGuard {
 public:
  explicit ClosureGuard(Closure* done) : done_(done) {}
  ~ClosureGuard() {
    if (done_) {
      done_->Run();
    }
  }

 private:
  Closure* done_;
};

}  // namespace storage