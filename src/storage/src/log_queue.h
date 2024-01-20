#pragma once

#include "rocksdb/status.h"

#include "pstd/noncopyable.h"
#include "pstd/thread_pool.h"

namespace storage {

class Task;

class LogQueue : public pstd::noncopyable {
 public:
  using WriteCallback = std::function<void(const Task&)>;

  explicit LogQueue(WriteCallback&& cb) : write_cb_(std::move(cb)) { consumer_.SetMaxIdleThread(1); }

  void Produce(Task&& task) { consumer_.ExecuteTask(write_cb_, task); }

 private:
  WriteCallback write_cb_;
  pstd::ThreadPool consumer_;
};

}  // namespace storage
