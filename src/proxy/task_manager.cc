#include <cassert>
#include <memory>
#include <condition_variable>

#include "task_manager.h"
#include "pstd/log.h"
#include "util.h"
#include "pikiwidb.h"

namespace pikiwidb {

void TaskManager::StartWorkers() {
  assert(state_ == State::kNone);
  
  size_t index = 1;
  while (worker_loops_.size() < worker_num_) {
    std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
    if (!name_.empty()) {
      loop->SetName(name_ + std::to_string(index++));
      INFO("loop {}, name {}", static_cast<void*>(loop.get()), loop->GetName().c_str());
    }
    worker_loops_.push_back(std::move(loop));
  }

  for (index = 0; index < worker_loops_.size(); ++index) {
    EventLoop* loop = worker_loops_[index].get();
    std::thread t([loop]() {
      loop->Init();
      loop->Run();
    });
    INFO("thread {}, thread loop {}, loop name {}", index, static_cast<void*>(loop), loop->GetName().c_str());
    worker_threads_.push_back(std::move(t));
  }

  state_ = State::kStarted;

  TaskMutex_.reserve(worker_num_);
  TaskCond_.reserve(worker_num_);
  TaskQueue_.reserve(worker_num_);
  
  for (size_t index = 0; index < worker_num_; ++index) {
    TaskMutex_.emplace_back(std::make_unique<std::mutex>());
    TaskCond_.emplace_back(std::make_unique<std::condition_variable>());
    TaskQueue_.emplace_back();
    
    std::thread t([this, index]() {
      while (TaskRunning_) {
        std::unique_lock lock(*TaskMutex_[index]);
        while (TaskQueue_[index].empty()) {
          if (!TaskRunning_) break;
          TaskCond_[index]->wait(lock);
        }
        if (!TaskRunning_) break;
        auto task = TaskQueue_[index].front();

        switch (task->GetTaskType()) {
        case ProxyBaseCmd::TaskType::kExecute:
          task->Execute();
          break;
        case ProxyBaseCmd::TaskType::kCallback: 
          task->CallBack();
          g_pikiwidb->PushWriteTask(task->Client());
          // return  DoCmd 之后有无返回，client 是否有 eventloop 维护
          break;
        default:
          ERROR("unsupported task type...");
          break;
        } 
        
        TaskQueue_[index].pop_front();
      }
      INFO("worker write thread {}, goodbye...", index);
    });

    INFO("worker write thread {}, starting...", index);
  }
  
}

void TaskManager::PushTask(std::shared_ptr<ProxyBaseCmd> task) {
  auto pos = (++t_counter_) % worker_num_;
  std::scoped_lock lock(*TaskMutex_[pos]);
  
  TaskQueue_[pos].emplace_back(task);
  TaskCond_[pos]->notify_one();
}

void TaskManager::Exit() {
  TaskRunning_ = false;

  int i = 0;
  for (auto& cond : TaskCond_) {
    std::scoped_lock lock(*TaskMutex_[i++]);
    cond->notify_all();
  }
  for (auto& wt : TaskThreads_) {
    if (wt.joinable()) {
      wt.join();
    }
  }

  TaskThreads_.clear();
  TaskCond_.clear();
  TaskQueue_.clear();
  TaskMutex_.clear();
}

}