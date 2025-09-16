// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_command_queue.h"
#include <glog/logging.h>

CommandQueue::CommandQueue(size_t max_size) : max_size_(max_size) {
    LOG(INFO) << "CommandQueue created with max_size: " << max_size_;
}

CommandQueue::~CommandQueue() {
    Shutdown();
    LOG(INFO) << "CommandQueue destroyed";
}

bool CommandQueue::EnqueueBatch(std::shared_ptr<CommandBatch> batch) {
    if (!batch || batch->Empty()) {
        LOG(WARNING) << "Attempt to enqueue empty or null batch";
        return false;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (shutdown_.load()) {
        LOG(WARNING) << "Cannot enqueue batch: queue is shutdown";
        return false;
    }
    
    if (cmd_queue_.size() >= max_size_) {
        LOG(WARNING) << "Command queue is full (size: " << cmd_queue_.size() 
                     << ", max: " << max_size_ << "), dropping batch";
        return false;
    }
    
    cmd_queue_.push(batch);
    
    //LOG(INFO) << "Enqueued command batch with " << batch->Size() 
              //<< " commands, queue size: " << cmd_queue_.size();
    
    queue_cv_.notify_one();
    return true;
}

std::shared_ptr<CommandBatch> CommandQueue::DequeueBatch() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    while (cmd_queue_.empty() && !shutdown_.load()) {
        queue_cv_.wait(lock);
    }
    
    if (shutdown_.load() && cmd_queue_.empty()) {
        return nullptr;
    }
    
    auto batch = cmd_queue_.front();
    cmd_queue_.pop();
    
   //LOG(INFO) << "Dequeued command batch with " << batch->Size() 
              //<< " commands, remaining queue size: " << cmd_queue_.size();
    
    return batch;
}

std::vector<std::shared_ptr<CommandBatch>> CommandQueue::DequeueAllBatches() {
    std::vector<std::shared_ptr<CommandBatch>> batches;
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (shutdown_.load()) {
        return batches;
    }
    
    // Take all available batches
    while (!cmd_queue_.empty()) {
        batches.push_back(cmd_queue_.front());
        cmd_queue_.pop();
    }
    
    if (!batches.empty()) {
        size_t total_commands = 0;
        for (const auto& batch : batches) {
            total_commands += batch->Size();
        }
       // LOG(INFO) << "Dequeued all batches: " << batches.size() 
                //  << " batches with " << total_commands << " total commands";
    }
    
    return batches;
}

size_t CommandQueue::Size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return cmd_queue_.size();
}

bool CommandQueue::Empty() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return cmd_queue_.empty();
}

void CommandQueue::Shutdown() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    shutdown_.store(true);
    queue_cv_.notify_all();
    LOG(INFO) << "CommandQueue shutdown, remaining batches: " << cmd_queue_.size();
}
