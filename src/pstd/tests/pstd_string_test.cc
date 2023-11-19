// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <climits>

#include <gtest/gtest.h>
#include "pstd/pstd_string.h"

class StringTest : public ::testing::Test {};

TEST_F(StringTest, StringTrim) {
  ASSERT_EQ(pstd::StringTrim("   computer  "), "computer");
  ASSERT_EQ(pstd::StringTrim("  comp  uter  "), "comp  uter");
  ASSERT_EQ(pstd::StringTrim(" \n  computer \n ", "\n "), "computer");
  ASSERT_EQ(pstd::StringTrim("\n", "\r\n "), "");
}

TEST_F(StringTest, ParseIpPort) {
  std::string ip;
  int port;
  ASSERT_TRUE(pstd::ParseIpPortString("192.168.1.1:9221", ip, port));
  ASSERT_EQ(ip, "192.168.1.1");
  ASSERT_EQ(port, 9221);
}

TEST_F(StringTest, test_string2ll) {
  char buf[32];
  long long v;

  /* May not start with +. */
  strcpy(buf, "+1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);

  /* Leading space. */
  strcpy(buf, " 1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);

  /* Trailing space. */
  strcpy(buf, "1 ");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);

  strcpy(buf, "-1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, -1);

  strcpy(buf, "0");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 0);

  strcpy(buf, "1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 1);

  strcpy(buf, "99");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 99);

  strcpy(buf, "-99");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, -99);

  strcpy(buf, "-9223372036854775808");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, LLONG_MIN);

  strcpy(buf, "-9223372036854775809"); /* overflow */
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);

  strcpy(buf, "9223372036854775807");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, LLONG_MAX);

  strcpy(buf, "9223372036854775808"); /* overflow */
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);
}

TEST_F(StringTest, test_string2l) {
  char buf[32];
  long v;

  /* May not start with +. */
  strcpy(buf, "+1");
  ASSERT_EQ(pstd::String2int(buf, &v), 0);

  strcpy(buf, "-1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, -1);

  strcpy(buf, "0");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 0);

  strcpy(buf, "1");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 1);

  strcpy(buf, "99");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, 99);

  strcpy(buf, "-99");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, -99);

#if LONG_MAX != LLONG_MAX
  strcpy(buf, "-2147483648");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, LONG_MIN);

  strcpy(buf, "-2147483649"); /* overflow */
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);

  strcpy(buf, "2147483647");
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 1);
  ASSERT_EQ(v, LONG_MAX);

  strcpy(buf, "2147483648"); /* overflow */
  ASSERT_EQ(pstd::String2int(buf, strlen(buf), &v), 0);
#endif
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}