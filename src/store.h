/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#define GLOG_NO_ABBREVIATED_SEVERITIES

#include "common.h"
#include "db.h"
#include "storage/storage.h"

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace pikiwidb {

class PStore {
 public:
  static PStore& Instance();

  PStore(const PStore&) = delete;
  void operator=(const PStore&) = delete;

  void Init(int dbNum);

  std::unique_ptr<DB>& GetBackend(int32_t index) { return backends_[index]; };

  std::shared_mutex& SharedMutex() { return dbs_mutex_; }

 private:
  PStore() = default;

  /**
   * If you want to access all the DBs at the same time,
   * then you must hold the lock.
   * For example: you want to execute flushall or bgsave.
   */
  std::shared_mutex dbs_mutex_;
  std::vector<std::unique_ptr<DB>> backends_;
};

#define PSTORE PStore::Instance()

}  // namespace pikiwidb
