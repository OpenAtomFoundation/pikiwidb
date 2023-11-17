// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <chrono>
#include <random>

namespace pstd {

extern std::mt19937 gen;

// init random seed
void InitRandom();

// return [0, max) random number
int RandomInt(int max);

// return [min, max] random number
int RandomInt(int min, int max);

// Returns an out-of-order vector of length n, and the numbers in the vector are unique

template <std::integral T>
std::vector<T> RandomPerm(T n) {
  if (n <= 0) {
    return {};
  }
  std::vector<T> perm(n);
  for (T i = 0; i < n; ++i) {
    perm[i] = i;
  }
  std::shuffle(perm.begin(), perm.end(), gen);
  return perm;
}

// return [0, 1] random double
double RandomDouble();

// returns t as a Unix time, the number of elapsed since January 1, 1970 UTC.
inline int64_t UnixTimestamp() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

// returns t as a Unix time, the number of milliseconds elapsed since January 1, 1970 UTC.
inline int64_t UnixMilliTimestamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// returns t as a Unix time, the number of Microseconds elapsed since January 1, 1970 UTC.
inline int64_t UnixMicroTimestamp() {
  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// returns t as a Unix time, the number of nanoseconds elapsed since January 1, 1970 UTC.
inline int64_t UnixNanoTimestamp() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace pstd