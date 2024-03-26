/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "checkpoint_manager.h"
#include "db.h"
#include "log.h"
#include "pstd/env.h"

namespace pikiwidb {

class DB;

void CheckpointManager::Init(int instNum, DB* db) {
  checkpoint_num_ = instNum;
  checkpoint_infoes_.resize(checkpoint_num_);
  res_.reserve(checkpoint_num_);
  db_ = db;
}

void CheckpointManager::CreateCheckpoint(const std::string& path) {
  res_.clear();
  std::lock_guard Lock(shared_mutex_);
  for (int i = 0; i < checkpoint_num_; ++i) {
    checkpoint_infoes_[i].checkpoint_in_process = true;
    auto res = std::async(std::launch::async, &DB::DoBgSave, db_, std::ref(checkpoint_infoes_[i]), path, i);
    res_.push_back(std::move(res));
  }
}

void CheckpointManager::WaitForCheckpointDone() {
  for (auto& r : res_) {
    r.get();
  }
}

}  // namespace pikiwidb