/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#define GLOG_NO_ABBREVIATED_SEVERITIES

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <set>

#include "common.h"
#include "db.h"
#include "checkpoint_manager.h"
#include "storage/storage.h"

namespace pikiwidb {
enum TaskType {
  kBgSave,
};

struct TaskContext {
  TaskType type;
  std::set<int> dbs;
  std::vector<std::string> argv;
  bool sync;
  TaskContext(TaskType t, bool sync = false) : type(t), sync(sync) {}
  TaskContext(TaskType t, const std::set<int>& d, bool sync = false) : type(t), dbs(d), sync(sync) {}
  TaskContext(TaskType t, const std::set<int>& d, const std::vector<std::string>& a, bool sync = false) : type(t), dbs(d), argv(a), sync(sync){}
};

class CheckpointManager;

class PStore {
 public:
  friend class CheckpointManager;
  static PStore& Instance();

  PStore(const PStore&) = delete;
  void operator=(const PStore&) = delete;

  void Init();

  std::unique_ptr<DB>& GetBackend(int32_t index) { return backends_[index]; };

  void DoSameThingSpecificDB(const TaskContext task);

  void FinishCheckpoint(bool sync);

 private:
  PStore() = default;

  int dbNum_ = 0;
  std::vector<std::unique_ptr<DB>> backends_;

  PString dumpPath_; // 由配置文件传入，当前为 ./dump
};

#define PSTORE PStore::Instance()

}  // namespace pikiwidb
