#pragma once

#include <optional>
#include <vector>

#include "rocksdb/slice.h"

namespace storage {

using Slice = rocksdb::Slice;

enum class OperateType { kNoOperate = 0, kPut, kDelete };
enum DataType { kAll, kStrings, kHashes, kLists, kZSets, kSets };

struct BinlogEntry {
  OperateType op_type_;
  Slice key_;
  std::optional<Slice> value_;
};

class Binlog {
 public:
  explicit Binlog(DataType type) : data_type_(type) {}

  void AppendOperate(OperateType type, const Slice& key, const std::optional<Slice>& value = std::nullopt) {
    entries_.emplace_back(type, key, value);
  }

  auto Serialization() -> std::string;
  static auto DeSerialization(const std::string&) -> Binlog;

  std::vector<BinlogEntry> entries_;
  DataType data_type_;

  static constexpr uint64_t kMagic_ = 0x12345678;
};

}  // namespace storage
