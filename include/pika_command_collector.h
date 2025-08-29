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

*/
class PikaCommandCollector {
 public:
  // Callback function type after command processing is completed
  using CommandCallback = std::function<void(const LogOffset& offset, pstd::Status status)>;

  /**
    * @brief constructor
    * @param coordinator consensus coordinator reference
    * @param batch_max_wait_time maximum wait time in milliseconds
  */
  // Constructor with raw pointer (original)
  PikaCommandCollector(ConsensusCoordinator* coordinator, int batch_max_wait_time = 5);
  
  // Constructor with shared_ptr (for compatibility with make_shared calls)
  PikaCommandCollector(std::shared_ptr<ConsensusCoordinator> coordinator, int batch_max_wait_time = 5);
  
  ~PikaCommandCollector();

  /**
    * @brief Add command to collector
    * @param cmd_ptr command pointer
    * @param callback callback function after processing is completed
    * @return whether the addition was successful
  */
  bool AddCommand(const std::shared_ptr<Cmd>& cmd_ptr, CommandCallback callback);

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

 private:
  //Consensus coordinator reference
  ConsensusCoordinator* coordinator_;

  // Batch processing configuration
  std::atomic<int> batch_max_wait_time_;
  
  // Batch statistics
  std::atomic<uint64_t> total_processed_{0};
  std::atomic<uint64_t> total_batches_{0};
};

#endif  // PIKA_COMMAND_COLLECTOR_H_