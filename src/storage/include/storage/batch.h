// Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <memory>
#include <functional>
#include <future>
#include <chrono>
#include <map>
#include "rocksdb/db.h"
#include "rocksdb/status.h"

// Forward declarations
namespace pikiwidb {
class Binlog;
}

class Cmd;

namespace net {
class NetConn;
}

namespace storage {

class Storage;
class Redis;

// Callback for async commit result
// Parameters: status (result should be captured in the lambda from storage layer)
// Note: Connection and result should be captured in the lambda, not passed as parameters
using CommitCallback = std::function<void(rocksdb::Status)>;

using AppendLogFunction = std::function<void(const ::pikiwidb::Binlog&, std::promise<rocksdb::Status>&&, 
                                             CommitCallback)>;

using ColumnFamilyIndex = uint32_t;

class Batch {
public:
  virtual ~Batch() = default;
  
  virtual void Put(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key, const rocksdb::Slice& value) = 0;
  
  virtual void Delete(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key) = 0;
  
  virtual rocksdb::Status Commit(CommitCallback callback = nullptr) = 0;
  
  int32_t Count() const { return count_; }
  
  static std::unique_ptr<Batch> CreateBatch(Redis* redis);

protected:
  int32_t count_ = 0;
};

class RocksBatch : public Batch {
public:
  RocksBatch(rocksdb::DB* db, 
             const rocksdb::WriteOptions& options,
             const std::vector<rocksdb::ColumnFamilyHandle*>& handles);
  
  void Put(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key, const rocksdb::Slice& value) override;
  void Delete(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key) override;
  rocksdb::Status Commit(CommitCallback callback = nullptr) override;

private:
  rocksdb::WriteBatch batch_;
  rocksdb::DB* db_;
  const rocksdb::WriteOptions& options_;
  std::vector<rocksdb::ColumnFamilyHandle*> handles_; 
};

class BinlogBatch : public Batch {
public:
  BinlogBatch(AppendLogFunction func, uint32_t data_type, uint32_t db_id, uint32_t slot_idx = 0, uint32_t timeout_s = 10);
  ~BinlogBatch() override;
  
  void Put(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key, const rocksdb::Slice& value) override;
  void Delete(ColumnFamilyIndex cf_idx, const rocksdb::Slice& key) override;
  
  // Commit to Raft (sync mode if callback is null, async mode otherwise)
  rocksdb::Status Commit(CommitCallback callback = nullptr) override;

private:
  AppendLogFunction append_log_func_;
  std::unique_ptr<::pikiwidb::Binlog> binlog_;
  uint32_t data_type_;  // 数据类型，用于 binlog entry
  uint32_t timeout_seconds_;
};

} // namespace storage
