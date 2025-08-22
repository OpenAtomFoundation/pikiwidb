// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKA_COMMAND_COLLECTOR_H_
#define PIKA_COMMAND_COLLECTOR_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <optional>

#include "include/pika_command.h"
#include "include/pika_define.h"
#include "pstd/include/pstd_status.h"

#include "include/pika_consensus.h"

/**
  * @brief PikaCommandCollector is used to collect write commands and process them in batches
  *
  * Main functions:
  * 1. Collect write commands and process them in optimized batches after reaching the threshold
  * 2. Handle the conflict of the same key (the later command will overwrite the earlier command)
  * 3. Send commands in batches to the consensus coordinator with batch-level synchronization
  * 4. Support asynchronous callback notification of command processing results
  * 5. Track performance metrics for batch processing
  * 6. Provide intelligent retry mechanisms for failed batches
*/
class PikaCommandCollector {
 public:
  // Callback function type after command processing is completed
  using CommandCallback = std::function<void(const LogOffset& offset, pstd::Status status)>;

  /**
    * @brief constructor
    * @param coordinator consensus coordinator reference
    * @param batch_size batch size (number of commands)
    * @param batch_max_wait_time forced flush interval (milliseconds)
  */
  // Constructor with raw pointer (original)
  PikaCommandCollector(ConsensusCoordinator* coordinator, size_t batch_size = 100, int batch_max_wait_time = 5);
  
  // Constructor with shared_ptr (for compatibility with make_shared calls)
  PikaCommandCollector(std::shared_ptr<ConsensusCoordinator> coordinator, size_t batch_size = 100, int batch_max_wait_time = 5);
  

  ~PikaCommandCollector();

  /**
    * @brief Add command to collector
    * @param cmd_ptr command pointer
    * @param callback callback function after processing is completed
    * @return whether the addition was successful
  */
  bool AddCommand(const std::shared_ptr<Cmd>& cmd_ptr, CommandCallback callback);

  /**
    * @brief Called periodically by external systems to process batches
    * @param force Force processing even if batch is not full or timeout not reached
    * @return Number of commands processed
  */

  /**
    * @brief Immediately process all currently collected commands
    * @return The number of commands processed
  */
  size_t FlushCommands(bool force = false);


  /**
    * @brief Get the current number of pending commands
    * @return number of commands
  */
  size_t PendingCommands() const;

  /**
    * @brief Set the batch size
    * @param batch_size batch size
  */
  void SetBatchSize(size_t batch_size);
  
  /**
    * @brief Set the batch max wait time
    * @param batch_max_wait_time maximum wait time in milliseconds
  */
  void SetBatchMaxWaitTime(int batch_max_wait_time);

  /**
    * @brief Get batch processing statistics
    * @return Pair of (total_processed_commands, total_batches)
  */
  std::pair<uint64_t, uint64_t> GetBatchStats() const;
  
  /**
    * @brief Get average batch processing time in milliseconds
    * @return Average processing time or nullopt if no batches processed
  */
  std::optional<double> GetAverageBatchTime() const;

 private:

  /**
    * @brief batch processing command
    * @param batch command batch
    * @return Whether the processing is successful
  */
  pstd::Status ProcessBatch(const std::vector<std::shared_ptr<Cmd>>& commands, 
                           const std::vector<CommandCallback>& callbacks);

  /**
    * @brief Check for conflicts based on command type and key name
    * @param cmd_ptr command pointer
    * @return true if there is a conflict (should be replaced), false if there is no conflict
  */
  bool CheckConflict(const std::shared_ptr<Cmd>& cmd_ptr) const;

  /**
    * @brief Handle key conflicts and remove conflicting commands
    * @param cmd_ptr new command
  */
  void HandleConflict(const std::shared_ptr<Cmd>& cmd_ptr);

  /**
    * @brief Retry batch processing commands
    * @param commands List of commands to retry
    * @param callbacks Corresponding callback function list
    * @param priority Priority level for the retry (higher means more urgent)
    * @return Whether the commands were successfully requeued
  */
  bool RetryBatch(const std::vector<std::shared_ptr<Cmd>>& commands,
                 const std::vector<CommandCallback>& callbacks,
                 int priority = 100);

 private:
  //Consensus coordinator reference
  ConsensusCoordinator* coordinator_;

  // Batch processing configuration
  std::atomic<size_t> batch_size_;
  std::atomic<int> batch_max_wait_time_;
  
  // Retry configuration
  std::atomic<int> max_retry_attempts_{3};
  std::atomic<int> retry_backoff_ms_{50};

  // Command collection and processing
  mutable std::mutex mutex_;
  
  // Pending command queue and callbacks
  std::list<std::pair<std::shared_ptr<Cmd>, CommandCallback>> pending_commands_;
  
  // Priority queue for retries
  std::deque<std::tuple<int, std::vector<std::shared_ptr<Cmd>>, std::vector<CommandCallback>>> retry_queue_;
  
  // Command key mapping, used to handle same-key conflicts
  std::unordered_map<std::string, std::list<std::pair<std::shared_ptr<Cmd>, CommandCallback>>::iterator> key_map_;

  // Batch statistics
  std::atomic<uint64_t> total_processed_{0};
  std::atomic<uint64_t> total_batches_{0};
  std::atomic<uint64_t> total_retries_{0};
  std::atomic<uint64_t> total_conflicts_{0};
  std::atomic<uint64_t> total_batch_time_ms_{0};
  std::chrono::time_point<std::chrono::steady_clock> batch_start_time_;
  
  // Performance tracking
  struct BatchMetrics {
    uint64_t batch_size;
    uint64_t processing_time_ms;
    uint64_t wait_time_ms;
    bool successful;
  };
  
  // Circular buffer for recent batch metrics
  static constexpr size_t kMetricsBufferSize = 100;
  std::vector<BatchMetrics> recent_metrics_;
  std::mutex metrics_mutex_;
};

#endif  // PIKA_COMMAND_COLLECTOR_H_