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
#include <set>
#include <shared_mutex>
#include <vector>

#include "checkpoint_manager.h"
#include "common.h"
#include "db.h"
#include "storage/storage.h"

namespace pikiwidb {

enum TaskType {
  kCheckpoint,
};

enum TaskArg {
  kCheckpointPath,
};

struct TaskContext {
  TaskType type;
  int db;
  std::map<TaskArg, std::string> args;
  TaskContext(TaskType t) : type(t) {}
  TaskContext(TaskType t, int d) : type(t), db(d) {}
  TaskContext(TaskType t, int d, const std::map<TaskArg, std::string>& a) : type(t), db(d), args(a) {}
};

using TasksVector = std::vector<TaskContext>;

class CheckpointManager;

class PStore {
 public:
  friend class CheckpointManager;
  static PStore& Instance();

  PStore(const PStore&) = delete;
  void operator=(const PStore&) = delete;

  void Init();

  std::unique_ptr<DB>& GetBackend(int32_t index) { return backends_[index]; };

  void DoSomeThingSpecificDB(const TasksVector task);

  void WaitForCheckpointDone();

  std::shared_mutex& SharedMutex() { return dbs_mutex_; }

 private:
  PStore() = default;
  void trimSlash(std::string& dirName);

  int dbNum_ = 0;

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
