/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_thread_pool_worker.h"
#include "log.h"
#include "pikiwidb.h"

namespace pikiwidb {

void CmdWorkThreadPoolWorker::Work() {
  while (running_) {
    LoadWork();
    for (const auto &task : selfTask) {
      if (task->Client()->State() != ClientState::kOK) {  // the client is closed
        continue;
      }
      auto [cmdPtr, ret] = cmd_table_manager_.GetCommand(task->CmdName(), task->Client().get());

      if (!cmdPtr) {
        if (ret == CmdRes::kInvalidParameter) {
          task->Client()->SetRes(CmdRes::kInvalidParameter);
        } else {
          task->Client()->SetRes(CmdRes::kSyntaxErr, "unknown command '" + task->CmdName() + "'");
        }
        g_pikiwidb->PushWriteTask(task->Client());
        continue;
      }

      if (!cmdPtr->CheckArg(task->Client()->ParamsSize())) {
        task->Client()->SetRes(CmdRes::kWrongNum, task->CmdName());
        g_pikiwidb->PushWriteTask(task->Client());
        continue;
      }
      task->Run(cmdPtr);
      g_pikiwidb->PushWriteTask(task->Client());
    }
    selfTask.clear();
  }
  INFO("slow worker [{}] goodbye...", name_);
}

void CmdWorkThreadPoolWorker::Stop() { running_ = false; }

void CmdFastWorker::LoadWork() {
  std::unique_lock lock(pool_->fastMutex_);
  while (pool_->fastTasks_.empty()) {
    if (!running_) {
      return;
    }
    pool_->fastCondition_.wait(lock);
  }

  const auto num = std::min(static_cast<int>(pool_->fastTasks_.size()), onceTask_);
  if (num > 0) {
    std::move(pool_->fastTasks_.begin(), pool_->fastTasks_.begin() + num, std::back_inserter(selfTask));
    pool_->fastTasks_.erase(pool_->fastTasks_.begin(), pool_->fastTasks_.begin() + num);
  }
}

void CmdSlowWorker::LoadWork() {
  {
    std::unique_lock lock(pool_->slowMutex_);
    while (pool_->slowTasks_.empty() && loopMore) {  // loopMore is used to get the fast worker
      if (!running_) {
        return;
      }
      pool_->slowCondition_.wait_for(lock, std::chrono::milliseconds(waitTime));
      loopMore = false;
    }

    const auto num = std::min(static_cast<int>(pool_->slowTasks_.size()), onceTask_);
    if (num > 0) {
      std::move(pool_->slowTasks_.begin(), pool_->slowTasks_.begin() + num, std::back_inserter(selfTask));
      pool_->slowTasks_.erase(pool_->slowTasks_.begin(), pool_->slowTasks_.begin() + num);
      return;  // If the slow task is obtained, the fast task is no longer obtained
    }
  }

  {
    std::unique_lock lock(pool_->fastMutex_);
    loopMore = true;

    const auto num = std::min(static_cast<int>(pool_->fastTasks_.size()), onceTask_);
    if (num > 0) {
      std::move(pool_->fastTasks_.begin(), pool_->fastTasks_.begin() + num, std::back_inserter(selfTask));
      pool_->fastTasks_.erase(pool_->fastTasks_.begin(), pool_->fastTasks_.begin() + num);
    }
  }
}

}  // namespace pikiwidb