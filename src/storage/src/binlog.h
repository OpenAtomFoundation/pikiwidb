#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include "rocksdb/slice.h"

#include "binlog.pb.h"

namespace storage {

using Slice = rocksdb::Slice;

class BinlogWrapper {
 public:
  explicit BinlogWrapper(BinlogDataType type) { binlog_->set_data_type(type); }

  void AppendPutOperation(int32_t cfid, const Slice& key, const Slice& value) {
    auto entry = binlog_->add_entries();
    entry->set_cf_idx(cfid);
    entry->set_op_type(OperateType::kPut);
    entry->set_key(key.ToString());
    entry->set_value(value.ToString());
  }
  void AppendDeleteOperation(int32_t cfid, const Slice& key) {
    auto entry = binlog_->add_entries();
    entry->set_cf_idx(cfid);
    entry->set_op_type(OperateType::kDelete);
    entry->set_key(key.ToString());
  }

  auto Serialization() const -> std::string { return binlog_->SerializeAsString(); }
  static auto Deserialization(const std::string& data) -> std::optional<Binlog> {
    Binlog log;
    auto res = log.ParseFromString(data);
    return res ? std::make_optional<Binlog>(std::move(log)) : std::nullopt;
  }

  auto MoveBinlog() -> Binlog&& { return std::move(*binlog_); }

 private:
  std::unique_ptr<Binlog> binlog_{std::make_unique<Binlog>()};
};

// enum class OperateType { kNoOperate = 0, kPut, kDelete };
// enum DataType { kAll, kStrings, kHashes, kLists, kZSets, kSets };

// struct BinlogEntry {
//   int32_t cf_idx_;
//   OperateType op_type_;
//   std::string key_;
//   std::optional<std::string> value_;
// };

// class Binlog {
//  public:
//   explicit Binlog(DataType type) : data_type_(type) {}

//   // void AppendOperation(int32_t cfid, OperateType type, const Slice& key,
//   //                      const std::optional<Slice>& value = std::nullopt) {
//   //   entries_.emplace_back(cfid, type, key, value);
//   // }
//   void AppendPutOperation(int32_t cfid, const Slice& key, const Slice& value) {
//     entries_.emplace_back(cfid, OperateType::kPut, key.ToString(), value.ToString());
//   }
//   void AppendDeleteOperation(int32_t cfid, const Slice& key) {
//     entries_.emplace_back(cfid, OperateType::kDelete, key.ToString(), std::nullopt);
//   }

//   auto Serialization() const -> std::string;
//   static auto DeSerialization(const std::string& data) -> std::optional<Binlog>;

//   std::vector<BinlogEntry> entries_;
//   DataType data_type_;
// };

// inline std::ostream& operator<<(std::ostream& os, const Binlog& log) {
//   os << "{ type=" << DataTypeTag[log.data_type_] << ", op_size=" << log.entries_.size() << ": ";
//   for (const auto& entry : log.entries_) {
//     if (entry.op_type_ == OperateType::kPut) {
//       os << "put (" << entry.key_ << ", " << *entry.value_ << "), ";
//     } else {
//       os << "del (" << entry.key_ << "), ";
//     }
//   }
//   os << "}";
//   return os;
// }

}  // namespace storage
