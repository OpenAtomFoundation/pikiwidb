/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <cstdlib>
#include <vector>
#include "pstring.h"

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
inline typename HASH::const_iterator KeyRandomHashMember(const HASH& container) {
  auto it = container.cend();
  if (container.empty()) {
    return it;
  }

  it = container.cbegin();
  // FIXME: don't iterator, use O(1) algorithm
  size_t randomIdx = random() % container.size();
  for (size_t i = 0; i < randomIdx; ++i) {
    ++it;
  }
  return it;
}

template <typename HASH>
inline size_t ScanHashMember(const HASH& container, size_t cursor, size_t count,
                             std::vector<typename HASH::const_iterator>& res) {
  // TODO(century): wait to do with folly concurrent hashmap
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
  VmPeak = 0,
  VmSize = 1,
  VmLck = 2,
  VmHWM = 3,
  VmRSS = 4,
  VmSwap = 5,

  VmMax = VmSwap + 1,
};

extern std::vector<size_t> getMemoryInfo();
extern size_t getMemoryInfo(MemoryInfoType type);

}  // namespace pikiwidb

