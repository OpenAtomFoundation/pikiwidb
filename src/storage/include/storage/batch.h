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
#include "binlog.pb.h"

namespace storage {

// 前置声明
class Storage;
enum DataType;  // 使用 storage.h 中定义的 DataType

// Binlog 提交回调函数类型（使用 promise/future 同步）
using AppendLogFunction = std::function<void(const pikiwidb::Binlog&, std::promise<rocksdb::Status>&&)>;

// Batch 抽象基类
class Batch {
public:
  virtual ~Batch() = default;
  
  // 添加 Put 操作
  virtual void Put(DataType dtype, const rocksdb::Slice& key, const rocksdb::Slice& value) = 0;
  
  // 添加 Delete 操作
  virtual void Delete(DataType dtype, const rocksdb::Slice& key) = 0;
  
  // 提交批量操作
  virtual rocksdb::Status Commit() = 0;
  
  // 获取操作计数
  int32_t Count() const { return count_; }
  
  // 静态工厂方法（参考 pikiwidb_raft 的 Batch::CreateBatch）
  static std::unique_ptr<Batch> CreateBatch(Storage* storage);

protected:
  int32_t count_ = 0;
};

// RocksDB 直接写入的 Batch（非 Raft 模式）
class RocksBatch : public Batch {
public:
  RocksBatch(Storage* storage,
             rocksdb::DB* strings_db, 
             rocksdb::DB* hashes_db,
             rocksdb::DB* lists_db,
             rocksdb::DB* sets_db,
             rocksdb::DB* zsets_db,
             rocksdb::DB* streams_db);
  
  void Put(DataType dtype, const rocksdb::Slice& key, const rocksdb::Slice& value) override;
  void Delete(DataType dtype, const rocksdb::Slice& key) override;
  rocksdb::Status Commit() override;

private:
  rocksdb::DB* GetDB(DataType dtype);
  
  Storage* storage_;  // 新增：保存 Storage 指针用于检查 Raft 模式
  rocksdb::DB* strings_db_;
  rocksdb::DB* hashes_db_;
  rocksdb::DB* lists_db_;
  rocksdb::DB* sets_db_;
  rocksdb::DB* zsets_db_;
  rocksdb::DB* streams_db_;
  
  // 每个数据类型一个 WriteBatch
  std::map<DataType, rocksdb::WriteBatch> batches_;
};

// Raft Binlog 生成的 Batch（Raft 模式）
class BinlogBatch : public Batch {
public:
  BinlogBatch(AppendLogFunction func, uint32_t db_id, uint32_t slot_idx = 0, uint32_t timeout_s = 10);
  
  void Put(DataType dtype, const rocksdb::Slice& key, const rocksdb::Slice& value) override;
  void Delete(DataType dtype, const rocksdb::Slice& key) override;
  
  // 同步等待 Raft 应用完成（使用 promise/future）
  rocksdb::Status Commit() override;

private:
  AppendLogFunction append_log_func_;
  pikiwidb::Binlog binlog_;
  uint32_t timeout_seconds_;
};

// Proto DataType 转 Storage DataType（声明）
DataType ProtoToStorageDataType(pikiwidb::DataType proto_type);

// Storage DataType 转 Proto DataType（声明）
pikiwidb::DataType StorageToProtoDataType(DataType dtype);

} // namespace storage

