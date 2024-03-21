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
  kBgSave,
};

struct TaskContext {
  TaskType type;
  std::set<int> dbs;
  std::vector<std::string> argv;
  bool sync;
  TaskContext(TaskType t, bool sync = false) : type(t), sync(sync) {}
  TaskContext(TaskType t, const std::set<int>& d, bool sync = false) : type(t), dbs(d), sync(sync) {}
  TaskContext(TaskType t, const std::set<int>& d, const std::vector<std::string>& a, bool sync = false)
      : type(t), dbs(d), argv(a), sync(sync) {}
};
class CheckpointManager;

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

  void WaitForCheckpointDone();

  std::shared_mutex& SharedMutex() { return dbs_mutex_; }


 private:
  PStore() = default;

  int dbNum_ = 0;
  PString dumpPath_;  // 由配置文件传入，当前为 ./dump

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
