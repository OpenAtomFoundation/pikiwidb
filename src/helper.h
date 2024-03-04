/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <random>
#include <string>
#include <vector>

#include "common.h"

namespace pikiwidb {

// hash func from redis
extern unsigned int dictGenHashFunction(const void* key, int len);

// hash function
struct my_hash {
  size_t operator()(const PString& str) const;
};

std::size_t BitCount(const uint8_t* buf, std::size_t len);

template <typename HASH>
inline typename HASH::const_local_iterator RandomHashMember(const HASH& container) {
  if (container.empty()) {
    return typename HASH::const_local_iterator();
  }

  while (true) {
    size_t bucket = random() % container.bucket_count();
    if (container.bucket_size(bucket) == 0) {
      continue;
    }

    long lucky = random() % container.bucket_size(bucket);
    typename HASH::const_local_iterator it = container.begin(bucket);
    while (lucky > 0) {
      ++it;
      --lucky;
    }

    return it;
  }

  return typename HASH::const_local_iterator();
}

template <typename HASH>
inline typename HASH::const_iterator FollyRandomHashMember(const HASH& container) {
  auto it = container.cend();
  if (container.empty()) {
    return it;
  }

  it = container.cbegin();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, container.size() - 1);

  size_t randomIdx = dis(gen);
  while (randomIdx > 0) {
    randomIdx--;
    ++it;
  }

  return it;
}

template <typename HASH>
inline size_t FollyScanHashMember(const HASH& container, size_t cursor, size_t count, std::vector<PString>& res) {
  if (cursor >= container.size()) {
    return 0;
  }

  // find corresponding iterator
  size_t idx = cursor;
  auto it = container.cbegin();
  while (idx > 0) {
    --idx;
    ++it;
  }

  size_t new_cursor = cursor;
  auto end = container.cend();
  while (res.size() < count && it != end) {
    ++new_cursor;
    res.push_back(it->first);
    ++it;
  }

  // not the last element of map
  if (it != end) {
    return new_cursor;
  }

  return 0;
}

// scan
template <typename HASH>
inline size_t ScanHashMember(const HASH& container, size_t cursor, size_t count,
                             std::vector<typename HASH::const_local_iterator>& res) {
  if (cursor >= container.size()) {
    return 0;
  }

  auto idx = cursor;
  for (decltype(container.bucket_count()) bucket = 0; bucket < container.bucket_count(); ++bucket) {
    const auto bktSize = container.bucket_size(bucket);
    if (idx < bktSize) {
      // find the bucket;
      auto it = container.begin(bucket);
      while (idx > 0) {
        ++it;
        --idx;
      }

      size_t newCursor = cursor;
      auto end = container.end(bucket);
      while (res.size() < count && it != end) {
        ++newCursor;
        res.push_back(it++);

        if (it == end) {
          while (++bucket < container.bucket_count()) {
            if (container.bucket_size(bucket) > 0) {
              it = container.begin(bucket);
              end = container.end(bucket);
              break;
            }
          }

          if (bucket == container.bucket_count()) {
            return 0;
          }
        }
      }

      return newCursor;
    } else {
      idx -= bktSize;
    }
  }

  return 0;  // never here
}

extern void getRandomHexChars(char* p, unsigned int len);

enum MemoryInfoType {
  kVmPeak = 0,
  kVmSize = 1,
  kVmLck = 2,
  kVmHWM = 3,
  kVmRSS = 4,
  kVmSwap = 5,

  kVmMax = kVmSwap + 1,
};

extern std::vector<size_t> getMemoryInfo();
extern size_t getMemoryInfo(MemoryInfoType type);

}  // namespace pikiwidb
