/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_thread_pool.h"
#include "cmd_thread_pool_worker.h"
#include "log.h"

namespace pikiwidb {

void CmdThreadPoolTask::Run(BaseCmd *cmd) { cmd->Execute(client_.get()); }
const std::string &CmdThreadPoolTask::CmdName() { return client_->CmdName(); }
std::shared_ptr<PClient> CmdThreadPoolTask::Client() { return client_; }

CmdThreadPool::CmdThreadPool(std::string name) : name_(std::move(name)) {}

pstd::Status CmdThreadPool::Init(int fastThread, int slowThread, std::string name) {
  if (fastThread < 0 || slowThread < 0) {
    return pstd::Status::InvalidArgument("thread num must be positive");
  }
  name_ = std::move(name);
  fastThreadNum_ = fastThread;
  slowThreadNum_ = slowThread;
  thread_.reserve(fastThreadNum_ + slowThreadNum_);
  return pstd::Status::OK();
}

void CmdThreadPool::Start() {
  for (int i = 0; i < fastThreadNum_; ++i) {
    std::jthread thread(&CmdWorkThreadPoolWorker::Work,
                        std::make_shared<CmdFastWorker>(this, 2, "fast worker" + std::to_string(i)));
    thread_.emplace_back(std::move(thread));
    INFO("fast worker [{}] starting ...", std::to_string(i));
  }
  for (int i = 0; i < slowThreadNum_; ++i) {
    std::jthread thread(&CmdWorkThreadPoolWorker::Work,
                        std::make_shared<CmdSlowWorker>(this, 2, "slow worker" + std::to_string(i)));
    thread_.emplace_back(std::move(thread));
    INFO("slow worker [{}] starting ...", std::to_string(i));
  }
}

void CmdThreadPool::SubmitFast(const std::shared_ptr<CmdThreadPoolTask> &runner) {
  std::unique_lock rl(fastMutex_);
  fastTasks_.emplace_back(runner);
  fastCondition_.notify_one();
}

void CmdThreadPool::SubmitSlow(const std::shared_ptr<CmdThreadPoolTask> &runner) {
  std::unique_lock rl(slowMutex_);
  slowTasks_.emplace_back(runner);
  slowCondition_.notify_one();
}

void CmdThreadPool::Stop() { doStop(); }

void CmdThreadPool::doStop() {
  if (stopped_.load()) {
    return;
  }
  stopped_.store(true);

  for (auto &thread : thread_) {
    thread.request_stop();
  }
}

CmdThreadPool::~CmdThreadPool() { doStop(); }

}  // namespace pikiwidb