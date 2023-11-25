// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pstd/pstd_util.h"
#include <gtest/gtest.h>

class UtilTest : public ::testing::Test {};

TEST(UtilTest, RandomInt) {
  int min = 0;
  int max = 100;
  int random_int = pstd::RandomInt(min, max);
  ASSERT_GE(random_int, min);
  ASSERT_LE(random_int, max);
}

TEST(UtilTest, RandomInt2) {
  int max = 100;
  for (int i = 0; i < 100; ++i) {
    int random_int = pstd::RandomInt(max);
    ASSERT_LE(random_int, max);
  }
}

TEST(UtilTest, RandomDouble) {
  double random_double = pstd::RandomDouble();
  ASSERT_GE(random_double, 0.0);
  ASSERT_LE(random_double, 1.0);
}

TEST(UtilTest, RandomPerm) {
  std::vector<int> perm = pstd::RandomPerm(10);
  ASSERT_EQ(perm.size(), 10);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(std::count(perm.begin(), perm.end(), i), 1);
  }
}

TEST(UtilTest, RandomPermEmpty) {
  std::vector<int> perm = pstd::RandomPerm(0);
  ASSERT_EQ(perm.size(), 0);
}

TEST(UtilTest, RandomPermNegative) {
  std::vector<int> perm = pstd::RandomPerm(-1);
  ASSERT_EQ(perm.size(), 0);
}

TEST(UtilTest, UnixTimestamp) {
  int64_t timestamp = pstd::UnixTimestamp();
  std::cout << timestamp << std::endl;
  ASSERT_GT(timestamp, 0);
}

TEST(UtilTest, UnixMilliTimestamp) {
  int64_t timestamp = pstd::UnixMilliTimestamp();
  std::cout << timestamp << std::endl;
  ASSERT_GT(timestamp, 0);
}

TEST(UtilTest, UnixMicroTimestamp) {
  int64_t timestamp = pstd::UnixMicroTimestamp();
  std::cout << timestamp << std::endl;
  ASSERT_GT(timestamp, 0);
}

TEST(UtilTest, UnixNanoTimestamp) {
  int64_t timestamp = pstd::UnixNanoTimestamp();
  std::cout << timestamp << std::endl;
  ASSERT_GT(timestamp, 0);
}

int main(int argc, char** argv) {
  pstd::InitRandom();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
