// Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include "storage/storage.h"  // 必须先包含以获取 DataType 定义
#include "storage/batch.h"
#include "glog/logging.h"
#include <sys/time.h>

namespace storage {

// ==================== Batch 工厂方法 ====================

std::unique_ptr<Batch> Batch::CreateBatch(Storage* storage) {
  if (storage->IsRaftEnabled()) {
    // Raft 模式：返回 BinlogBatch
    return std::make_unique<BinlogBatch>(
      storage->GetBinlogCallback(),
      0,  // db_id
      0,  // slot_idx
      10  // timeout_s
    );
  } else {
    // 直接写入模式：返回 RocksBatch
    return std::make_unique<RocksBatch>(
      storage->GetStringsDB(),
      storage->GetHashesDB(),
      storage->GetListsDB(),
      storage->GetSetsDB(),
      storage->GetZSetsDB(),
      storage->GetStreamsDB()
    );
  }
}

// ==================== RocksBatch 实现 ====================

RocksBatch::RocksBatch(rocksdb::DB* strings_db,
                       rocksdb::DB* hashes_db,
                       rocksdb::DB* lists_db,
                       rocksdb::DB* sets_db,
                       rocksdb::DB* zsets_db,
                       rocksdb::DB* streams_db)
    : strings_db_(strings_db),
      hashes_db_(hashes_db),
      lists_db_(lists_db),
      sets_db_(sets_db),
      zsets_db_(zsets_db),
      streams_db_(streams_db) {
}

rocksdb::DB* RocksBatch::GetDB(DataType dtype) {
  switch (dtype) {
    case kStrings: return strings_db_;
    case kHashes: return hashes_db_;
    case kLists: return lists_db_;
    case kSets: return sets_db_;
    case kZSets: return zsets_db_;
    case kStreams: return streams_db_;
    default:
      LOG(ERROR) << "Unknown DataType: " << static_cast<int>(dtype);
      return nullptr;
  }
}

void RocksBatch::Put(DataType dtype, const rocksdb::Slice& key, const rocksdb::Slice& value) {
  batches_[dtype].Put(key, value);
  count_++;
}

void RocksBatch::Delete(DataType dtype, const rocksdb::Slice& key) {
  batches_[dtype].Delete(key);
  count_++;
}

rocksdb::Status RocksBatch::Commit() {
  rocksdb::WriteOptions write_options;
  
  // 对每个数据类型提交其 WriteBatch
  for (auto& pair : batches_) {
    DataType dtype = pair.first;
    rocksdb::WriteBatch& batch = pair.second;
    
    if (batch.Count() == 0) {
      continue;
    }
    
    rocksdb::DB* db = GetDB(dtype);
    if (!db) {
      return rocksdb::Status::InvalidArgument("Invalid DataType");
    }
    
    rocksdb::Status s = db->Write(write_options, &batch);
    if (!s.ok()) {
      LOG(ERROR) << "Failed to write batch for DataType " << static_cast<int>(dtype)
                 << ": " << s.ToString();
      return s;
    }
  }
  
  return rocksdb::Status::OK();
}

// ==================== BinlogBatch 实现 ====================

BinlogBatch::BinlogBatch(AppendLogFunction func, uint32_t db_id, uint32_t slot_idx, uint32_t timeout_s)
    : append_log_func_(std::move(func)),
      timeout_seconds_(timeout_s) {
  binlog_.set_db_id(db_id);
  binlog_.set_slot_idx(slot_idx);
}

void BinlogBatch::Put(DataType dtype, const rocksdb::Slice& key, const rocksdb::Slice& value) {
  auto* entry = binlog_.add_entries();
  entry->set_data_type(StorageToProtoDataType(dtype));
  entry->set_op_type(pikiwidb::OperateType::kPut);
  entry->set_key(key.data(), key.size());
  entry->set_value(value.data(), value.size());
  
  // 设置时间戳
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  entry->set_timestamp(static_cast<uint64_t>(tv.tv_sec));
  
  count_++;
}

void BinlogBatch::Delete(DataType dtype, const rocksdb::Slice& key) {
  auto* entry = binlog_.add_entries();
  entry->set_data_type(StorageToProtoDataType(dtype));
  entry->set_op_type(pikiwidb::OperateType::kDelete);
  entry->set_key(key.data(), key.size());
  
  // 设置时间戳
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  entry->set_timestamp(static_cast<uint64_t>(tv.tv_sec));
  
  count_++;
}

rocksdb::Status BinlogBatch::Commit() {
  if (count_ == 0) {
    return rocksdb::Status::OK();
  }
  
  // TODO: 填充 Raft 和 Binlog 元信息
  // binlog_.set_term(/* current term */);
  // binlog_.set_log_index(/* current index */);
  // binlog_.set_filenum(/* current filenum */);
  // binlog_.set_offset(/* current offset */);
  
  // 使用 promise/future 同步等待 Raft 应用完成
  std::promise<rocksdb::Status> promise;
  auto future = promise.get_future();
  
  // 调用回调函数（传递 binlog 和 promise）
  append_log_func_(binlog_, std::move(promise));
  
  // 等待结果（带超时）
  auto status = future.wait_for(std::chrono::seconds(timeout_seconds_));
  if (status == std::future_status::timeout) {
    LOG(ERROR) << "Raft apply timeout after " << timeout_seconds_ << " seconds";
    return rocksdb::Status::TimedOut("Wait for Raft apply timeout");
  }
  
  // 获取实际结果
  return future.get();
}

// ==================== 辅助转换函数 ====================

// Proto DataType 转 Storage DataType
// Proto: kStrings=0, kHashes=1, kLists=2, kSets=3, kZSets=4, kStreams=5
// Storage: kAll=0, kStrings=1, kHashes=2, kLists=3, kZSets=4, kSets=5, kStreams=6
DataType ProtoToStorageDataType(pikiwidb::DataType proto_type) {
  switch (proto_type) {
    case pikiwidb::DataType::kStrings: return kStrings;
    case pikiwidb::DataType::kHashes: return kHashes;
    case pikiwidb::DataType::kLists: return kLists;
    case pikiwidb::DataType::kSets: return kSets;
    case pikiwidb::DataType::kZSets: return kZSets;
    case pikiwidb::DataType::kStreams: return kStreams;
    default: return kAll;  // 不应该发生
  }
}

// Storage DataType 转 Proto DataType
pikiwidb::DataType StorageToProtoDataType(DataType dtype) {
  switch (dtype) {
    case kStrings: return pikiwidb::DataType::kStrings;
    case kHashes: return pikiwidb::DataType::kHashes;
    case kLists: return pikiwidb::DataType::kLists;
    case kSets: return pikiwidb::DataType::kSets;
    case kZSets: return pikiwidb::DataType::kZSets;
    case kStreams: return pikiwidb::DataType::kStreams;
    default: return pikiwidb::DataType::kStrings;  // 默认
  }
}

} // namespace storage

