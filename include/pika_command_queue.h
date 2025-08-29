// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKA_COMMAND_QUEUE_H_
#define PIKA_COMMAND_QUEUE_H_

#include <memory>
#include <queue>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "pstd/include/pstd_mutex.h"
#include "pstd/include/env.h"
#include "include/pika_command.h"
#include "include/pika_define.h"
#include "pstd/include/env.h"
#include "include/pika_define.h"

// Callback function type for command completion notification
using CommandCallback = std::function<void(const LogOffset&, pstd::Status)>;

// Structure representing a batch of commands
struct CommandBatch {
  std::vector<std::shared_ptr<Cmd>> commands;
  std::vector<CommandCallback> callbacks;
  uint64_t batch_id;
  uint64_t create_time;
  std::string db_name;
  std::vector<LogOffset> binlog_offsets;  // Binlog offsets for each command
  
  CommandBatch(const std::vector<std::shared_ptr<Cmd>>& cmds,
               const std::vector<CommandCallback>& cbs,
               const std::string& db)
      : commands(cmds), callbacks(cbs), db_name(db) {
    static std::atomic<uint64_t> next_id{1};
    batch_id = next_id.fetch_add(1);
    create_time = pstd::NowMicros();
  }
  
  bool Empty() const {
    return commands.empty();
  }
  
  size_t Size() const {
    return commands.size();
  }
};

// New structure to group multiple CommandBatches for RocksDB processing
struct BatchGroup {
  std::vector<std::shared_ptr<CommandBatch>> batches;
  LogOffset end_offset;  // Only store the final offset of the last batch
  BatchGroup() = default;
  BatchGroup(const std::vector<std::shared_ptr<CommandBatch>>& batches, 
             const LogOffset& final_offset)
      : batches(batches), end_offset(final_offset) {}
  bool Empty() const { return batches.empty(); }
  size_t BatchCount() const { return batches.size(); }
};

// Thread-safe command queue for batched command processing
class CommandQueue {
public:
  explicit CommandQueue(size_t max_size);
  ~CommandQueue();
  
  // Enqueue a command batch (blocking if queue is full)
  bool EnqueueBatch(std::shared_ptr<CommandBatch> batch);
  
  // Dequeue a command batch (blocking if queue is empty)
  std::shared_ptr<CommandBatch> DequeueBatch();
  
  // Dequeue all available batches (non-blocking)
  std::vector<std::shared_ptr<CommandBatch>> DequeueAllBatches();
  
  // Get current queue size
  size_t Size() const;
  
  // Check if queue is empty
  bool Empty() const;
  
  // Shutdown the queue
  void Shutdown();

private:
  std::queue<std::shared_ptr<CommandBatch>> cmd_queue_;
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  size_t max_size_;
  std::atomic<bool> shutdown_{false};
};

#endif  // PIKA_COMMAND_QUEUE_H_