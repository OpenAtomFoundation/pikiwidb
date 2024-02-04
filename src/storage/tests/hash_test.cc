#include <chrono>
#include <iostream>

#include "gtest/gtest.h"

#include "storage/storage.h"
#include "storage/util.h"

using namespace storage;

class TimerGuard {
 public:
  TimerGuard(const std::string& name = "") : name_(name) { start_ = std::chrono::steady_clock::now(); }
  ~TimerGuard() {
    Stop();
    std::cout << name_ << " cost " << std::chrono::duration<double>{end_ - start_}.count() << "s" << std::endl;
  }

 private:
  void Stop() { end_ = std::chrono::steady_clock::now(); }
  std::string name_;
  std::chrono::steady_clock::time_point start_;
  std::chrono::steady_clock::time_point end_;
};

class HashTest : public ::testing::Test {
 public:
  HashTest() {
    if (access(db_path_.c_str(), F_OK) != 0) {
      mkdir(db_path_.c_str(), 0755);
    }
    options_.options.create_if_missing = true;
  }
  ~HashTest() { DeleteFiles(db_path_.c_str()); }

  std::string db_path_{"./test_db/hash"};
  StorageOptions options_;
  Storage db_;
  uint32_t test_times_ = 100000;
  std::string key_ = "hash-test";
  std::string field_prefix_ = "field";
  std::string value_prefix_ = "value";
};

TEST_F(HashTest, DoNothing) {}

TEST_F(HashTest, HSetUseRocksBatchWithWAL) {
  options_.is_write_by_binlog = false;
  auto s = db_.Open(options_, db_path_);
  EXPECT_TRUE(s.ok());

  {
    TimerGuard timer("hset use RocksBatch with WAL");
    for (int i = 0; i < test_times_; i++) {
      auto field = field_prefix_ + std::to_string(i);
      auto value = value_prefix_ + std::to_string(i);
      int32_t res{};
      s = db_.HSet(key_, field, value, &res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(1, res);

      std::string get_res;
      s = db_.HGet(key_, field, &get_res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(value, get_res);
    }
  }
}

TEST_F(HashTest, HSetUseRocksBatchWithoutWAL) {
  options_.is_write_by_binlog = false;
  auto s = db_.Open(options_, db_path_);
  EXPECT_TRUE(s.ok());

  db_.DisableWal(true);
  {
    TimerGuard timer("hset use RocksBatch without WAL");
    for (int i = 0; i < test_times_; i++) {
      auto field = field_prefix_ + std::to_string(i);
      auto value = value_prefix_ + std::to_string(i);
      int32_t res{};
      s = db_.HSet(key_, field, value, &res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(1, res);

      std::string get_res;
      s = db_.HGet(key_, field, &get_res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(value, get_res);
    }
  }
}

TEST_F(HashTest, HSetUseBinlogBatchWithWAL) {
  options_.is_write_by_binlog = true;
  auto s = db_.Open(options_, db_path_);
  EXPECT_TRUE(s.ok());

  {
    TimerGuard timer("hset use BinlogBatch with WAL");
    for (int i = 0; i < test_times_; i++) {
      auto field = field_prefix_ + std::to_string(i);
      auto value = value_prefix_ + std::to_string(i);
      int32_t res{};
      s = db_.HSet(key_, field, value, &res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(1, res);

      std::string get_res;
      s = db_.HGet(key_, field, &get_res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(value, get_res);
    }
  }
}

TEST_F(HashTest, HSetUseBinlogBatchWithoutWAL) {
  options_.is_write_by_binlog = true;
  auto s = db_.Open(options_, db_path_);
  EXPECT_TRUE(s.ok());

  db_.DisableWal(true);
  {
    TimerGuard timer("hset use BinlogBatch without WAL");
    for (int i = 0; i < test_times_; i++) {
      auto field = field_prefix_ + std::to_string(i);
      auto value = value_prefix_ + std::to_string(i);
      int32_t res{};
      s = db_.HSet(key_, field, value, &res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(1, res);

      std::string get_res;
      s = db_.HGet(key_, field, &get_res);
      EXPECT_TRUE(s.ok());
      EXPECT_EQ(value, get_res);
    }
  }
}
