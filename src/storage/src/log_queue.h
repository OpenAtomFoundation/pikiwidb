#pragma once

#include "rocksdb/status.h"

#include "pstd/noncopyable.h"
#include "pstd/thread_pool.h"

namespace storage {

class LogQueue : public pstd::noncopyable {
 public:
  using WriteCallback = std::function<rocksdb::Status(const std::string&)>;

  explicit LogQueue(WriteCallback&& cb) : write_cb_(std::move(cb)) { consumer_.SetMaxIdleThread(1); }

  auto Produce(std::string&& data) -> std::future<rocksdb::Status> { return consumer_.ExecuteTask(write_cb_, data); }

 private:
  WriteCallback write_cb_;
  pstd::ThreadPool consumer_;
};

}  // namespace storage
