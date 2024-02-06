#pragma once

#include <atomic>
#include <cstdint>
#include "google/protobuf/stubs/callback.h"
#include "rocksdb/status.h"

#include "pstd/noncopyable.h"
#include "pstd/thread_pool.h"

namespace storage {

using Closure = ::google::protobuf::Closure;

class RocksClosure : public Closure {
 public:
  explicit RocksClosure(std::promise<rocksdb::Status>&& promise) : promise_(std::move(promise)) {}

  void Run() override { promise_.set_value(result_); }
  void SetStatus(rocksdb::Status&& status) { result_ = std::move(status); }

 private:
  std::promise<rocksdb::Status> promise_;
  rocksdb::Status result_{rocksdb::Status::Aborted("Unknown error")};
};

class ClosureGuard {
 public:
  explicit ClosureGuard(Closure* done) : done_(done) {}
  ~ClosureGuard() {
    if (done_) {
      done_->Run();
    }
  }

 private:
  Closure* done_;
};

class Task {
 public:
  Task(std::string&& data, std::promise<rocksdb::Status>&& promise)
      : data_(std::move(data)), done_(std::make_shared<RocksClosure>(std::move(promise))) {}

  int64_t log_idx_;
  std::string data_;
  std::shared_ptr<Closure> done_;
};

class TaskQueue : public pstd::noncopyable {
 public:
  using WriteCallback = std::function<void(const Task&)>;

  explicit TaskQueue(WriteCallback&& cb) : write_cb_(std::move(cb)) { consumer_.SetMaxIdleThread(1); }

  void Produce(Task&& task) {
    task.log_idx_ = next_idx_.fetch_add(1);
    consumer_.ExecuteTask(write_cb_, task);
  }

 private:
  std::atomic<int64_t> next_idx_{1};
  WriteCallback write_cb_;
  pstd::ThreadPool consumer_;
};

}  // namespace storage
