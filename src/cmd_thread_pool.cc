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
  if (fastThread < 0) {
    return pstd::Status::InvalidArgument("thread num must be positive");
  }
  name_ = std::move(name);
  fastThreadNum_ = fastThread;
  slowThreadNum_ = slowThread;
  threads_.reserve(fastThreadNum_ + slowThreadNum_);
  workers_.reserve(fastThreadNum_ + slowThreadNum_);
  return pstd::Status::OK();
}

void CmdThreadPool::Start() {
  for (int i = 0; i < fastThreadNum_; ++i) {
    auto fastWorker = std::make_shared<CmdFastWorker>(this, 2, "fast worker" + std::to_string(i));
    std::thread thread(&CmdWorkThreadPoolWorker::Work, fastWorker);
    threads_.emplace_back(std::move(thread));
    workers_.emplace_back(fastWorker);
    INFO("fast worker [{}] starting ...", i);
  }
  for (int i = 0; i < slowThreadNum_; ++i) {
    auto slowWorker = std::make_shared<CmdSlowWorker>(this, 2, "slow worker" + std::to_string(i));
    std::thread thread(&CmdWorkThreadPoolWorker::Work, slowWorker);
    threads_.emplace_back(std::move(thread));
    workers_.emplace_back(slowWorker);
    INFO("slow worker [{}] starting ...", i);
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

  for (auto &worker : workers_) {
    worker->Stop();
  }

  {
    std::unique_lock fl(fastMutex_);
    fastCondition_.notify_all();
  }
  {
    std::unique_lock sl(slowMutex_);
    slowCondition_.notify_all();
  }

  for (auto &thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  threads_.clear();
  workers_.clear();
  fastTasks_.clear();
  slowTasks_.clear();
}

CmdThreadPool::~CmdThreadPool() { doStop(); }

}  // namespace pikiwidb