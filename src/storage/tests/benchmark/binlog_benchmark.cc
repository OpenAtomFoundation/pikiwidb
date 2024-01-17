#include "storage/binlog.h"

#include "gtest/gtest.h"

using namespace storage;

TEST(BinlogTest, BinlogBenchmark) {
  constexpr int32_t times = 1000000;
  std::vector<BinlogEntry> test_entries = {
      {3, OperateType::kPut, "testkey1", "testvalue1"},
      {8, OperateType::kPut, "testkeya", "testvaluea"},
      {-1, OperateType::kDelete, "testkey*", "testvalue*"},
  };

  size_t sum_size{};
  for (int i = 0; i < times; i++) {
    Binlog log(DataType::kHashes);
    log.entries_ = test_entries;

    auto data = log.Serialization();
    sum_size += data.size();

    auto res = Binlog::DeSerialization(data);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->data_type_, kHashes);
    EXPECT_EQ(res->entries_.size(), test_entries.size());
    for (size_t i = 0; i < res->entries_.size(); i++) {
      EXPECT_EQ(res->entries_[i].cf_idx_, test_entries[i].cf_idx_);
      EXPECT_EQ(res->entries_[i].op_type_, test_entries[i].op_type_);
      EXPECT_EQ(res->entries_[i].key_, test_entries[i].key_);
      EXPECT_EQ(res->entries_[i].value_, test_entries[i].value_);
    }
  }
  std::cerr << "Average Size: " << sum_size / times << std::endl;
}

static auto RandomEntry() -> BinlogEntry {
  BinlogEntry entry;
  entry.cf_idx_ = rand() % 5;
  entry.op_type_ = rand() % 2 == 0 ? OperateType::kPut : OperateType::kDelete;
  entry.key_ = "key" + std::to_string(rand() % 1000);
  if (entry.op_type_ == OperateType::kPut) {
    entry.value_ = "value" + std::to_string(rand() % 1000);
  } else {
    entry.value_ = std::nullopt;
  }
  return entry;
}

static auto RandomBinlog() -> Binlog {
  Binlog log(static_cast<DataType>(rand() % 5 + 1));
  int32_t op_num = rand() % 7;
  for (int i = 0; i < op_num; i++) {
    log.entries_.push_back(RandomEntry());
  }
  return log;
}

TEST(BinlogTest, BinlogRandomBenchmark) {
  constexpr int32_t times = 1000000;

  size_t sum_size{};
  for (int i = 0; i < times; i++) {
    auto log = RandomBinlog();
    auto data = log.Serialization();
    sum_size += data.size();

    auto res = Binlog::DeSerialization(data);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->data_type_, log.data_type_);
    EXPECT_EQ(res->entries_.size(), log.entries_.size());
    for (size_t i = 0; i < res->entries_.size(); i++) {
      EXPECT_EQ(res->entries_[i].cf_idx_, log.entries_[i].cf_idx_);
      EXPECT_EQ(res->entries_[i].op_type_, log.entries_[i].op_type_);
      EXPECT_EQ(res->entries_[i].key_, log.entries_[i].key_);
      EXPECT_EQ(res->entries_[i].value_, log.entries_[i].value_);
    }
  }
  std::cerr << "Average Size: " << sum_size / times << std::endl;
}