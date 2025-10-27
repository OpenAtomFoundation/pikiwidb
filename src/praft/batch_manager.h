// Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <future>
#include <chrono>
#include "rocksdb/status.h"
#include "braft/raft.h"

namespace pika_raft {

// 批处理配置
struct BatchConfig {
  int batch_timeout_ms = 2;      // 批处理时间窗口（默认 2ms）
  size_t max_batch_size = 1000;  // 最大批量大小
  bool enable_batching = true;    // 是否启用批处理
};

// 批处理项
struct BatchItem {
  std::string log_data;                                    // 日志数据
  std::shared_ptr<std::promise<rocksdb::Status>> promise; // 结果通知
  uint64_t submit_time_us;                                 // 提交时间（用于监控）
};

// 批量 Closure（用于批量通知所有 promise）
class BatchClosure : public braft::Closure {
public:
  BatchClosure(std::vector<std::shared_ptr<std::promise<rocksdb::Status>>> promises)
      : promises_(std::move(promises)) {}
  
  void Run() override;

private:
  std::vector<std::shared_ptr<std::promise<rocksdb::Status>>> promises_;
};

// 批处理管理器
class BatchManager {
public:
  explicit BatchManager(braft::Node* raft_node, const BatchConfig& config = BatchConfig());
  ~BatchManager();
  
  // 提交单个命令到批处理队列
  void Submit(const std::string& log_data, 
              std::shared_ptr<std::promise<rocksdb::Status>> promise);
  
  // 启动批处理线程
  void Start();
  
  // 停止批处理线程
  void Stop();
  
  // 获取统计信息
  struct Stats {
    uint64_t total_requests = 0;
    uint64_t total_batches = 0;
    double avg_batch_size = 0.0;
    uint64_t pending_count = 0;
  };
  Stats GetStats() const;

private:
  // 批处理工作线程
  void BatchWorker();
  
  // 提交一批请求到 Raft
  void FlushBatch();
  
  braft::Node* raft_node_;
  BatchConfig config_;
  
  // 待处理队列
  std::vector<BatchItem> pending_items_;
  std::mutex mutex_;
  std::condition_variable cv_;
  
  // 工作线程
  std::thread batch_thread_;
  std::atomic<bool> running_{false};
  
  // 统计信息
  std::atomic<uint64_t> total_requests_{0};
  std::atomic<uint64_t> total_batches_{0};
};

} // namespace pika_raft

