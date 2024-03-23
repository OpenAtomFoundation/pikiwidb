/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef PIKIWIDB_CHECKPOINT_MANAGER_H
#define PIKIWIDB_CHECKPOINT_MANAGER_H

#include <future>
#include <shared_mutex>
#include <thread>
#include <utility>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/utilities/checkpoint.h"

namespace pikiwidb {

class DB;

struct CheckpointInfo {
  bool checkpoint_in_process = false;
};

class CheckpointManager {
 public:
  CheckpointManager() = default;
  ~CheckpointManager() = default;
  ;

  void Init(int instNum, DB* db);

  void CreateCheckpoint(const std::string& path);

  void WaitForCheckpointDone();

  bool CheckpointInProcess();

 private:
  int checkpoint_num_;
  std::vector<std::future<void>> res_;
  DB* db_ = nullptr;

  std::shared_mutex shared_mutex_;
  std::vector<CheckpointInfo> checkpoint_infoes_;
};
}  // namespace pikiwidb

#endif  // PIKIWIDB_CHECKPOINT_MANAGER_H
