#include "binlog.h"

#include "gtest/gtest.h"

using namespace storage;

// TEST(BinlogTest, BinlogBenchmark) {
//   constexpr int32_t times = 1000000;
//   std::vector<BinlogEntry> test_entries = {
//       {3, OperateType::kPut, "testkey1", "testvalue1"},
//       {8, OperateType::kPut, "testkeya", "testvaluea"},
//       {-1, OperateType::kDelete, "testkey*", "testvalue*"},
//   };

//   size_t sum_size{};
//   for (int i = 0; i < times; i++) {
//     Binlog log(DataType::kHashes);
//     log.entries_ = test_entries;

//     auto data = log.Serialization();
//     sum_size += data.size();

//     auto res = Binlog::DeSerialization(data);
//     EXPECT_TRUE(res.has_value());
//     EXPECT_EQ(res->data_type_, kHashes);
//     EXPECT_EQ(res->entries_.size(), test_entries.size());
//     for (size_t i = 0; i < res->entries_.size(); i++) {
//       EXPECT_EQ(res->entries_[i].cf_idx_, test_entries[i].cf_idx_);
//       EXPECT_EQ(res->entries_[i].op_type_, test_entries[i].op_type_);
//       EXPECT_EQ(res->entries_[i].key_, test_entries[i].key_);
//       EXPECT_EQ(res->entries_[i].value_, test_entries[i].value_);
//     }
//   }
//   std::cerr << "Average Size: " << sum_size / times << std::endl;
// }

static auto RandomEntry() -> BinlogEntry {
  BinlogEntry entry;
  entry.set_cf_idx(rand() % 5);
  entry.set_op_type(rand() % 2 == 0 ? OperateType::kPut : OperateType::kDelete);
  entry.set_key("key" + std::to_string(rand() % 1000));
  if (entry.op_type() == OperateType::kPut) {
    entry.set_value("value" + std::to_string(rand() % 1000));
  }
  return entry;
}

static auto RandomBinlog() -> BinlogWrapper {
  BinlogWrapper binlog(static_cast<BinlogDataType>(rand() % 5 + 1));

  int32_t op_num = rand() % 7;
  for (int i = 0; i < op_num; i++) {
    auto op_type = rand() % 2 == 0 ? OperateType::kPut : OperateType::kDelete;
    if (op_type == OperateType::kPut) {
      binlog.AppendPutOperation(rand() % 5, "key" + std::to_string(rand() % 1000),
                                "value" + std::to_string(rand() % 1000));
    } else {
      binlog.AppendDeleteOperation(rand() % 5, "key" + std::to_string(rand() % 1000));
    }
  }
  return binlog;
}

TEST(BinlogTest, BinlogRandomBenchmark) {
  constexpr int32_t times = 1000000;

  size_t sum_size{};
  for (int i = 0; i < times; i++) {
    auto wrapper = RandomBinlog();
    auto data = wrapper.Serialization();
    auto log = wrapper.MoveBinlog();
    sum_size += data.size();

    auto res = BinlogWrapper::Deserialization(data);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->data_type(), log.data_type());
    EXPECT_EQ(res->entries().size(), log.entries().size());
    for (size_t i = 0; i < res->entries().size(); i++) {
      EXPECT_EQ(res->entries()[i].cf_idx(), log.entries()[i].cf_idx());
      EXPECT_EQ(res->entries()[i].op_type(), log.entries()[i].op_type());
      EXPECT_EQ(res->entries()[i].key(), log.entries()[i].key());
      EXPECT_EQ(res->entries()[i].value(), log.entries()[i].value());
    }
  }
  std::cerr << "Average Size: " << sum_size / times << std::endl;
}
