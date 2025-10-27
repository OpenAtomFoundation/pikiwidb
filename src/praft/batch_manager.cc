// Copyright (c) 2024-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include "batch_manager.h"
#include <glog/logging.h>
#include "pstd/include/pstd_defer.h"

namespace pika_raft {

// BatchClosure 实现
void BatchClosure::Run() {
  std::unique_ptr<BatchClosure> self_guard(this);
  
  // 根据 Raft 结果通知所有 promise
  if (status().ok()) {
    for (auto& promise : promises_) {
      if (promise) {
        promise->set_value(rocksdb::Status::OK());
      }
    }
  } else {
    rocksdb::Status error_status = rocksdb::Status::IOError(status().error_str());
    for (auto& promise : promises_) {
      if (promise) {
        promise->set_value(error_status);
      }
    }
  }
}

// BatchManager 实现
BatchManager::BatchManager(braft::Node* raft_node, const BatchConfig& config)
    : raft_node_(raft_node), config_(config) {
  pending_items_.reserve(config_.max_batch_size);
}

BatchManager::~BatchManager() {
  Stop();
}

void BatchManager::Submit(const std::string& log_data,
                          std::shared_ptr<std::promise<rocksdb::Status>> promise) {
  if (!config_.enable_batching) {
    // 批处理未启用，直接提交
    auto* closure = new BatchClosure({promise});
    braft::Task task;
    butil::IOBuf buf;
    buf.append(log_data);
    task.data = &buf;
    task.done = closure;
    raft_node_->apply(task);
    return;
  }
  
  std::lock_guard<std::mutex> lock(mutex_);
  
  BatchItem item;
  item.log_data = log_data;
  item.promise = promise;
  item.submit_time_us = butil::gettimeofday_us();
  
  pending_items_.push_back(std::move(item));
  total_requests_.fetch_add(1, std::memory_order_relaxed);
  
  // 如果队列满了，立即通知工作线程
  if (pending_items_.size() >= config_.max_batch_size) {
    cv_.notify_one();
  }
}

void BatchManager::Start() {
  if (running_.exchange(true)) {
    return;  // 已经在运行
  }
  
  batch_thread_ = std::thread([this]() { BatchWorker(); });
  LOG(INFO) << "BatchManager started with config: timeout=" << config_.batch_timeout_ms 
            << "ms, max_batch_size=" << config_.max_batch_size;
}

void BatchManager::Stop() {
  if (!running_.exchange(false)) {
    return;  // 已经停止
  }
  
  cv_.notify_one();
  if (batch_thread_.joinable()) {
    batch_thread_.join();
  }
  
  LOG(INFO) << "BatchManager stopped. Total requests: " << total_requests_.load()
            << ", Total batches: " << total_batches_.load();
}

void BatchManager::BatchWorker() {
  LOG(INFO) << "BatchManager worker thread started";
  
  while (running_.load(std::memory_order_relaxed)) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // 等待有数据或超时
    cv_.wait_for(lock, std::chrono::milliseconds(config_.batch_timeout_ms), 
                 [this]() {
                   return !pending_items_.empty() || 
                          !running_.load(std::memory_order_relaxed);
                 });
    
    if (!running_.load(std::memory_order_relaxed)) {
      // 处理剩余请求
      if (!pending_items_.empty()) {
        FlushBatch();
      }
      break;
    }
    
    if (!pending_items_.empty()) {
      FlushBatch();
    }
  }
  
  LOG(INFO) << "BatchManager worker thread stopped";
}

void BatchManager::FlushBatch() {
  // 假设调用者已持有锁
  
  if (pending_items_.empty()) {
    return;
  }
  
  size_t batch_size = pending_items_.size();
  
  // 合并所有日志数据
  // 格式：<count>\n<log1_len>\n<log1_data><log2_len>\n<log2_data>...
  std::string batch_data;
  batch_data.reserve(batch_size * 100);  // 预估大小
  
  // 写入批次大小
  batch_data.append(std::to_string(batch_size));
  batch_data.append("\n");
  
  // 收集所有 promise
  std::vector<std::shared_ptr<std::promise<rocksdb::Status>>> promises;
  promises.reserve(batch_size);
  
  for (auto& item : pending_items_) {
    // 写入单个日志长度和数据
    batch_data.append(std::to_string(item.log_data.size()));
    batch_data.append("\n");
    batch_data.append(item.log_data);
    
    promises.push_back(std::move(item.promise));
  }
  
  pending_items_.clear();
  
  // 创建批量 closure
  auto* closure = new BatchClosure(std::move(promises));
  
  // 提交到 Raft
  braft::Task task;
  butil::IOBuf buf;
  buf.append(batch_data);
  task.data = &buf;
  task.done = closure;
  
  raft_node_->apply(task);
  
  total_batches_.fetch_add(1, std::memory_order_relaxed);
  
  VLOG(1) << "Flushed batch of " << batch_size << " requests to Raft";
}

BatchManager::Stats BatchManager::GetStats() const {
  Stats stats;
  stats.total_requests = total_requests_.load(std::memory_order_relaxed);
  stats.total_batches = total_batches_.load(std::memory_order_relaxed);
  
  if (stats.total_batches > 0) {
    stats.avg_batch_size = static_cast<double>(stats.total_requests) / stats.total_batches;
  }
  
  std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
  stats.pending_count = pending_items_.size();
  
  return stats;
}

} // namespace pika_raft

