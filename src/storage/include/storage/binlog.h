#pragma once

#include <vector>

#include "rocksdb/slice.h"

namespace pikiwidb {

using Slice = rocksdb::Slice;

enum class OperateType { kNoOperate = 0, kPut, kDelete };
enum class DataType { kString = 0, kList, kHash, kSet, kZSet };

struct BinlogEntry {
  OperateType op_type_;
  DataType data_type_;
  Slice key_;
  Slice value_;
};

class Binlog {
 public:
  auto Serialization() -> std::string;
  static auto DeSerialization(const std::string&) -> Binlog;

 private:
  std::vector<BinlogEntry> entries_;

  static constexpr uint64_t kMagic_ = 0x12345678;
};

}  // namespace pikiwidb
