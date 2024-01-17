#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include "binlog.pb.h"
#include "rocksdb/slice.h"

namespace storage {

using Slice = rocksdb::Slice;

enum class OperateType { kNoOperate = 0, kPut, kDelete };
enum DataType { kAll, kStrings, kHashes, kLists, kZSets, kSets };
const char DataTypeTag[] = {'a', 'k', 'h', 'l', 'z', 's'};

struct BinlogEntry {
  int32_t cf_idx_;
  OperateType op_type_;
  std::string key_;
  std::optional<std::string> value_;
};

class Binlog {
 public:
  explicit Binlog(DataType type) : data_type_(type) {}

  // void AppendOperation(int32_t cfid, OperateType type, const Slice& key,
  //                      const std::optional<Slice>& value = std::nullopt) {
  //   entries_.emplace_back(cfid, type, key, value);
  // }
  void AppendPutOperation(int32_t cfid, const Slice& key, const Slice& value) {
    entries_.emplace_back(cfid, OperateType::kPut, key.ToString(), value.ToString());
  }
  void AppendDeleteOperation(int32_t cfid, const Slice& key) {
    entries_.emplace_back(cfid, OperateType::kDelete, key.ToString(), std::nullopt);
  }

  auto Serialization() const -> std::string;
  static auto DeSerialization(const BinlogProto& proto) -> Binlog;
  static auto GetBinlogProto(const std::string&) -> std::optional<BinlogProto>;

  std::vector<BinlogEntry> entries_;
  DataType data_type_;
};

inline std::ostream& operator<<(std::ostream& os, const Binlog& log) {
  os << "{ type=" << DataTypeTag[log.data_type_] << ", op_size=" << log.entries_.size() << ": ";
  for (const auto& entry : log.entries_) {
    if (entry.op_type_ == OperateType::kPut) {
      os << "put (" << entry.key_ << ", " << *entry.value_ << "), ";
    } else {
      os << "del (" << entry.key_ << "), ";
    }
  }
  os << "}";
  return os;
}

}  // namespace storage
