#include "storage/binlog.h"

#include "binlog.pb.h"

namespace storage {

auto Binlog::Serialization() const -> std::string {
  BinlogProto binlog_proto;
  binlog_proto.set_data_type(static_cast<DataTypeProto>(data_type_));
  for (const auto& entry : entries_) {
    auto ptr = binlog_proto.add_entries();
    ptr->set_cf_idx(entry.cf_idx_);
    ptr->set_op_type(static_cast<OperateTypeProto>(entry.op_type_));
    ptr->set_key(entry.key_.ToString());
    ptr->set_value(entry.value_->ToString());
  }
  return binlog_proto.SerializeAsString();
}

auto Binlog::DeSerialization(const std::string& data) -> std::optional<Binlog> {
  BinlogProto binlog_proto;
  if (!binlog_proto.ParseFromString(data)) {
    return std::nullopt;
  }
  Binlog binlog(static_cast<DataType>(binlog_proto.data_type()));
  binlog.entries_.resize(binlog_proto.entries_size());
  auto& entries = binlog.entries_;
  const auto& protos = binlog_proto.entries();
  for (size_t i = 0; i < entries.size(); i++) {
    entries[i].cf_idx_ = protos[i].cf_idx();
    entries[i].op_type_ = static_cast<OperateType>(protos[i].op_type());
    entries[i].key_ = protos[i].key();
    entries[i].value_ = protos[i].value();
  }
  return binlog;
}

}  // namespace storage
