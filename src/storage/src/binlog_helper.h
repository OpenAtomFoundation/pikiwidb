#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include "rocksdb/slice.h"

#include "binlog.pb.h"

namespace storage::BinlogHelper {

using Slice = rocksdb::Slice;

inline auto CreateBinlog(BinlogDataType type) -> Binlog {
  Binlog log;
  log.set_data_type(type);
  return log;
}

inline void AppendPutOperation(Binlog& log, int32_t cf_idx, const Slice& key, const Slice& value) {
  auto entry = log.add_entries();
  entry->set_cf_idx(cf_idx);
  entry->set_op_type(OperateType::kPut);
  entry->set_key(key.ToString());
  entry->set_value(value.ToString());
}

inline void AppendDeleteOperation(Binlog& log, int32_t cf_idx, const Slice& key) {
  auto entry = log.add_entries();
  entry->set_cf_idx(cf_idx);
  entry->set_op_type(OperateType::kDelete);
  entry->set_key(key.ToString());
}

}  // namespace storage::BinlogHelper
