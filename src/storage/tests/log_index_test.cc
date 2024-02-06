#include <memory>
#include <string>
#include "fmt/core.h"
#include "gtest/gtest.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "src/log_index.h"
#include "src/redis.h"

using namespace storage;  // NOLINT

TEST(TablePropertyTest, SimpleTest) {
  constexpr const char* kDbPath = "./test_db/tmp";
  rocksdb::Options options;
  options.create_if_missing = true;
  LogIndexAndSequenceCollector collector;
  options.table_properties_collector_factories.push_back(
      std::make_shared<LogIndexTablePropertiesCollectorFactory>(&collector));
  rocksdb::DB* db{nullptr};
  auto s = rocksdb::DB::Open(options, kDbPath, &db);
  EXPECT_TRUE(s.ok());

  collector.Update(233333, 322222);
  db->Flush(rocksdb::FlushOptions());
  std::string property;
  auto res = db->GetProperty(LogIndexTablePropertiesCollector::kPropertyName_, &property);
  EXPECT_TRUE(res);
  fmt::println("{}: {}", LogIndexTablePropertiesCollector::kPropertyName_, property);

  db->Close();
  DeleteFiles(kDbPath);
}

class LogIndexTest : public ::testing::Test {
 public:
  LogIndexTest() {
    if (access(db_path_.c_str(), F_OK) != 0) {
      mkdir(db_path_.c_str(), 0755);
    }
    options_.options.create_if_missing = true;
  }
  ~LogIndexTest() override { DeleteFiles(db_path_.c_str()); }

  std::string db_path_{"./test_db/log_index_test"};
  StorageOptions options_;
  Storage db_;
  uint32_t test_times_ = 100;
  std::string key_ = "log-index-test";
  std::string field_prefix_ = "field";
  std::string value_prefix_ = "value";
  rocksdb::WriteOptions write_options_;
  rocksdb::ReadOptions read_options_;
};

TEST_F(LogIndexTest, DISABLED_SimpleTest) {  // NOLINT
  options_.is_write_by_binlog = true;
  auto s = db_.Open(options_, db_path_);
  EXPECT_TRUE(s.ok());

  auto& redis = db_.GetDBInstance(key_);
  for (int i = 0; i < test_times_; i++) {
    auto field = field_prefix_ + std::to_string(i);
    auto value = value_prefix_ + std::to_string(i);
    int32_t res{};
    s = redis->HSet(key_, field, value, &res);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(1, res);

    std::string get_res;
    s = redis->HGet(key_, field, &get_res);
    EXPECT_TRUE(s.ok());
    EXPECT_EQ(value, get_res);
  }
  redis->GetDB()->Flush(rocksdb::FlushOptions());
  std::string property;
  auto res = redis->GetDB()->GetProperty(LogIndexTablePropertiesCollector::kPropertyName_, &property);
  EXPECT_TRUE(res);
  fmt::println("{}: {}", LogIndexTablePropertiesCollector::kPropertyName_, property);
}