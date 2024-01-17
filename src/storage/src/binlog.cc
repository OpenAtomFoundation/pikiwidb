#include "storage/binlog.h"

namespace storage {

auto Binlog::Serialization() const -> std::string {
  BinlogProto binlog_proto;
  binlog_proto.set_data_type(static_cast<DataTypeProto>(data_type_));
  for (const auto& entry : entries_) {
    auto ptr = binlog_proto.add_entries();
    ptr->set_cf_idx(entry.cf_idx_);
    ptr->set_op_type(static_cast<OperateTypeProto>(entry.op_type_));
    ptr->set_key(entry.key_);
    if (entry.value_.has_value()) {
      ptr->set_value(*entry.value_);
    }
  }
  return binlog_proto.SerializeAsString();
}

auto Binlog::DeSerialization(const BinlogProto& proto) -> Binlog {
  Binlog binlog(static_cast<DataType>(proto.data_type()));
  binlog.entries_.resize(proto.entries_size());
  auto& entries = binlog.entries_;
  const auto& protos = proto.entries();
  for (size_t i = 0; i < entries.size(); i++) {
    entries[i].cf_idx_ = protos[i].cf_idx();
    entries[i].op_type_ = static_cast<OperateType>(protos[i].op_type());
    entries[i].key_ = protos[i].key();
    if (protos[i].has_value()) {
      entries[i].value_ = protos[i].value();
    }
  }
  return binlog;
}

auto Binlog::GetBinlogProto(const std::string& data) -> std::optional<BinlogProto> {
  BinlogProto binlog_proto;
  if (!binlog_proto.ParseFromString(data)) {
    return std::nullopt;
  }
  return binlog_proto;
}

}  // namespace storage
