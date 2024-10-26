#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <thread>

#include "client.h"
#include "proxy_base_cmd.h"
#include "cmd_thread_pool.h"
#include "net/event_loop.h"
#include "base_cmd.h"
#include "cmd_table_manager.h"
#include "io_thread_pool.h"

namespace pikiwidb {


class TaskManager : public IOThreadPool {
public:
  TaskManager() = default;
  ~TaskManager() = default;
  
  static size_t GetMaxWorkerNum() { return kMaxWorkers; }
  
  void Exit() override;
  void PushTask(std::shared_ptr<ProxyBaseCmd> task);

private:
  void StartWorkers() override;    

private:
  std::vector<std::thread> TaskThreads_;
  std::vector<std::unique_ptr<std::mutex>> TaskMutex_;
  std::vector<std::unique_ptr<std::condition_variable>> TaskCond_;
  std::vector<std::deque<std::shared_ptr<ProxyBaseCmd>>> TaskQueue_;
  
  std::atomic<uint64_t> t_counter_ = 0;
  bool TaskRunning_ = true;
};

} // namespace pikiwidb