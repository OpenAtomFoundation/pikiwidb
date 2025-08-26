// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_command_collector.h"
#include "include/pika_conf.h"
#include "include/pika_server.h"
#include "include/pika_rm.h"
#include "include/pika_define.h"
#include <glog/logging.h>
#include <chrono>

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaReplicaManager> g_pika_rm;

PikaCommandCollector::PikaCommandCollector(ConsensusCoordinator* coordinator, size_t batch_size, int batch_max_wait_time)
    : coordinator_(coordinator),
      batch_size_(batch_size),
      batch_max_wait_time_(batch_max_wait_time) {
  
  // Check if coordinator is null
  if (!coordinator_) {
    LOG(FATAL) << "PikaCommandCollector: ConsensusCoordinator cannot be null! "
               << "This usually means SyncMasterDB is not initialized yet.";
    return;
  }
  
  LOG(INFO) << "PikaCommandCollector created with batch_size=" << batch_size << ", batch_max_wait_time=" << batch_max_wait_time << "ms";
  
  // Initialize metrics buffer
  recent_metrics_.reserve(kMetricsBufferSize);
}

// Constructor with shared_ptr (for compatibility with make_shared calls)
PikaCommandCollector::PikaCommandCollector(std::shared_ptr<ConsensusCoordinator> coordinator, size_t batch_size, int batch_max_wait_time)
    : coordinator_(coordinator.get()),
      batch_size_(batch_size),
      batch_max_wait_time_(batch_max_wait_time) {
  
  // Check if coordinator is null
  if (!coordinator_) {
    LOG(FATAL) << "PikaCommandCollector: ConsensusCoordinator cannot be null! "
               << "This usually means SyncMasterDB is not initialized yet.";
    return;
  }
  
  LOG(INFO) << "PikaCommandCollector created from shared_ptr with batch_size=" << batch_size << ", batch_max_wait_time=" << batch_max_wait_time << "ms";
  
  // Initialize metrics buffer
  recent_metrics_.reserve(kMetricsBufferSize);
}

PikaCommandCollector::~PikaCommandCollector() {
  // Process any remaining commands
  FlushCommands(true);
  LOG(INFO) << "PikaCommandCollector stopped, processed " << total_processed_.load()
            << ", conflicts: " << total_conflicts_.load();
}

bool PikaCommandCollector::AddCommand(const std::shared_ptr<Cmd>& cmd_ptr, CommandCallback callback) {
  if (!cmd_ptr || !cmd_ptr->is_write()) {
    LOG(WARNING) << "Attempt to add non-write command to CommandCollector";
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  
  if (pending_commands_.empty()) {
    batch_start_time_ = std::chrono::steady_clock::now();
  }
  
  // Check if we should immediately flush the batch
  bool should_flush = pending_commands_.size() >= static_cast<size_t>(batch_size_.load());
  if (should_flush) {
    FlushCommands(false);
  }
  
  // Handle same Key conflict - counts updated inside HandleConflict
  HandleConflict(cmd_ptr);
  
  // Add command to queue
  pending_commands_.emplace_back(cmd_ptr, std::move(callback));
  
  // Update Key Mapping
  std::vector<std::string> keys = cmd_ptr->current_key();
  for (const auto& key : keys) {
    key_map_[key] = std::prev(pending_commands_.end());
  }

    
  return true;
}

size_t PikaCommandCollector::FlushCommands(bool force) {
  std::vector<std::shared_ptr<Cmd>> commands;
  std::vector<CommandCallback> callbacks;
  
  // Record batch metrics variables
  auto batch_start = std::chrono::steady_clock::now();
  uint64_t wait_time_ms = 0;
  bool batch_successful = false;
  
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_commands_.empty()) {
      // Check if there are any retries to process
      if (!retry_queue_.empty() && force) {
        auto& [pri, cmds, cbs] = retry_queue_.front();
        commands = cmds;
        callbacks = cbs;
        retry_queue_.pop_front();
        LOG(INFO) << "FlushCommands: Processing retry batch with priority " << pri << ", size: " << commands.size();
      } else {
        return 0;
      }
    } else {
      auto now = std::chrono::steady_clock::now();
      auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - batch_start_time_).count();
      wait_time_ms = elapsed_ms; // Record wait time for metrics
      
      bool should_flush = force ||
                          pending_commands_.size() >= static_cast<size_t>(batch_size_.load()) ||
                          elapsed_ms > batch_max_wait_time_.load();
                          
      if (!should_flush) {
        return 0;
      }
      
      size_t batch_count = pending_commands_.size();
      if (!force) {
        batch_count = std::min(batch_count, static_cast<size_t>(batch_size_.load()));
      }

      commands.reserve(batch_count);
      callbacks.reserve(batch_count);
      
      auto it = pending_commands_.begin();
      for (size_t i = 0; i < batch_count; ++i, ++it) {
        commands.push_back(it->first);
        callbacks.push_back(std::move(it->second));
      }
      
      // Clear queue and map
      for (const auto& cmd : commands) {
        std::vector<std::string> keys = cmd->current_key();
        for (const auto& key : keys) {
          key_map_.erase(key);
        }
      }
      pending_commands_.erase(pending_commands_.begin(), std::next(pending_commands_.begin(), batch_count));
      
      if (!pending_commands_.empty()) {
        // Reset timer for the next batch
        batch_start_time_ = std::chrono::steady_clock::now();
      }
    }
  }
  
  size_t batch_size = commands.size();
  if (batch_size > 0) {
    LOG(INFO) << "Processing batch of " << batch_size << " commands";
    
    auto process_start = std::chrono::steady_clock::now();
    pstd::Status status = ProcessBatch(commands, callbacks);
    auto process_end = std::chrono::steady_clock::now();
    
    uint64_t processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(process_end - process_start).count();
    batch_successful = status.ok();
    
    if (!batch_successful) {
      LOG(ERROR) << "Error processing command batch: " << status.ToString();
    } else {
      LOG(INFO) << "Successfully processed batch in " << processing_time_ms << "ms";
    }
    
    // Update statistics
    total_processed_.fetch_add(batch_size);
    total_batches_.fetch_add(1);
    total_batch_time_ms_.fetch_add(processing_time_ms);
    
    // Record batch metrics
    {
      std::lock_guard<std::mutex> metrics_lock(metrics_mutex_);
      if (recent_metrics_.size() >= kMetricsBufferSize) {
        recent_metrics_.erase(recent_metrics_.begin());
      }
      recent_metrics_.push_back({batch_size, processing_time_ms, wait_time_ms, batch_successful});
    }
    
    // Process any retries if there were failures but we have pending retries
    if (!batch_successful) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!retry_queue_.empty()) {
        LOG(INFO) << "FlushCommands: Processing retries due to batch failure";
        // Schedule immediate follow-up flush to process retries
        return batch_size + FlushCommands(true);
      }
    }
  }
  
  return batch_size;
}

pstd::Status PikaCommandCollector::ProcessBatch(
    const std::vector<std::shared_ptr<Cmd>>& commands,
    const std::vector<CommandCallback>& callbacks) {
  
  if (commands.empty()) {
    return pstd::Status::OK();
  }
  
    // Implement batch processing logic here
  // 1. Generate binlogs for each command
  // 2. Write binlogs to production queue in batches
  // 3. Main node will update memory data structures in batches
  // 4. Trigger asynchronous persistence
  
  // Store the log offset for each command
  std::vector<LogOffset> offsets;
  
  // Check if coordinator is valid
  if (!coordinator_) {
    LOG(ERROR) << "ProcessBatch: ConsensusCoordinator is null";
    return pstd::Status::InvalidArgument("ConsensusCoordinator is null");
  }
  
  // Get SyncMasterDB and submit commands in batch
  DBInfo db_info(coordinator_->db_name());
  auto master_db = g_pika_rm->GetSyncMasterDBByName(db_info);
  if (!master_db) {
    LOG(ERROR) << "Failed to get SyncMasterDB for " << coordinator_->db_name();
    return pstd::Status::NotFound("SyncMasterDB not found");
  }
  
  // Submit to consensus coordinator in batch
  LOG(INFO) << "ProcessBatch: Processing " << commands.size() << " commands in batch";
  pstd::Status batch_status = master_db->ConsensusBatchProposeLog(commands, &offsets);
  
  // Log the batch status
  if (!batch_status.ok()) {
    LOG(WARNING) << "ProcessBatch: Batch operation failed with status: " << batch_status.ToString();
    if (batch_status.IsTimeout()) {
      LOG(WARNING) << "ProcessBatch: Timeout occurred, triggering batch retry mechanism";
      
      // Get the last command's offset
      LogOffset last_offset;
      if (!offsets.empty()) {
        last_offset = offsets.back();
      }
      
      // Roll back committed_id on master and all slave nodes
      if (last_offset.IsValid()) {
        LOG(WARNING) << "ProcessBatch: Rolling back committed_id to before " << last_offset.ToString();
        
        // Get current committed_id
        LogOffset current_committed_id = master_db->GetCommittedId();

        // Calculate rollback target committed_id (assuming rollback to the previous batch's committed_id)
        // In actual implementation, you may need to adjust the rollback target based on specific situations
        LogOffset rollback_target = current_committed_id;
        rollback_target.l_offset.index -= commands.size(); // Simple rollback, may need more complex logic in practice
        if (rollback_target.l_offset.index < 0) {
          rollback_target.l_offset.index = 0;
        }

        // Execute rollback operation
        LOG(WARNING) << "ProcessBatch: Rolling back from " << current_committed_id.ToString()
                    << " to " << rollback_target.ToString();

        // Call rollback API
        pstd::Status truncate_status = master_db->Truncate(rollback_target);
        if (!truncate_status.ok()) {
          LOG(ERROR) << "ProcessBatch: Failed to rollback committed_id: " << truncate_status.ToString();
        } else {
          LOG(INFO) << "ProcessBatch: Successfully rolled back committed_id";
          
          // Call batch retry mechanism
          LOG(INFO) << "ProcessBatch: Triggering batch retry mechanism for " << commands.size() << " commands";
          bool retry_result = RetryBatch(commands, callbacks, 100); // Use high priority for timeouts
          if (retry_result) {
            LOG(INFO) << "ProcessBatch: Successfully requeued commands for retry";
            // Already requeued, no need to execute callbacks
            return batch_status;
          } else {
            LOG(ERROR) << "ProcessBatch: Failed to requeue commands for retry";
          }
        }
      }
    }
  } else {
    LOG(INFO) << "ProcessBatch: Batch operation completed successfully";
  }
  
  // Execute callback for each command
  for (size_t i = 0; i < commands.size() && i < callbacks.size(); ++i) {
    if (callbacks[i]) {
      pstd::Status cmd_status;
      // If batch processing status failed, all commands should fail
      if (!batch_status.ok()) {
        // Pass the upper layer error status to the client
        cmd_status = batch_status;
      } else {
        // If the offset is empty, it means the command failed to be added
        cmd_status = offsets[i].IsValid() ? pstd::Status::OK() : pstd::Status::IOError("Failed to append command");
      }
      
      // Log information before executing each callback function
      LOG(INFO) << "ProcessBatch: Executing callback for command " << i 
                << ", cmd=" << (commands[i] ? commands[i]->name() : "null")
                << ", status=" << cmd_status.ToString()
                << ", offset=" << (offsets[i].IsValid() ? offsets[i].ToString() : "invalid");
      // Execute callback
      callbacks[i](offsets[i], cmd_status);
      
      // Log information after callback execution
      LOG(INFO) << "ProcessBatch: Callback executed for command " << i;
    }
  }
  return batch_status;
}

bool PikaCommandCollector::CheckConflict(const std::shared_ptr<Cmd>& cmd_ptr) const {
  if (!cmd_ptr) {
    return false;
  }
  
  std::vector<std::string> keys = cmd_ptr->current_key();
  for (const auto& key : keys) {
    if (key_map_.find(key) != key_map_.end()) {
      return true;
    }
  }
  
  return false;
}

void PikaCommandCollector::HandleConflict(const std::shared_ptr<Cmd>& cmd_ptr) {
  if (!cmd_ptr) {
    return;
  }
  
  std::vector<std::string> keys = cmd_ptr->current_key();
  std::vector<std::list<std::pair<std::shared_ptr<Cmd>, CommandCallback>>::iterator> to_remove;
  
  // Find all conflicting commands
  for (const auto& key : keys) {
    auto it = key_map_.find(key);
    if (it != key_map_.end()) {
      // Check if this iterator is already added
      bool already_added = false;
      for (const auto& iter : to_remove) {
        if (iter == it->second) {
          already_added = true;
          break;
        }
      }
      if (!already_added) {
        to_remove.push_back(it->second);
      }
      key_map_.erase(it);
    }
  }
  
  // Track conflict metrics
  if (!to_remove.empty()) {
    total_conflicts_.fetch_add(to_remove.size());
  }
  
  // Check command importance to prevent important commands from being overwritten
  for (auto it : to_remove) {
    auto old_cmd = it->first;
    auto new_cmd = cmd_ptr;
    
    // Determine command importance
    // If the old command type exists in the important command list, consider it important
    bool is_important_cmd = false;
    // Check commands with keywords such as "EXEC", "MULTI", "WATCH", etc.
    std::string cmd_name = old_cmd->name();
    if (cmd_name == "MULTI" || cmd_name == "EXEC" || cmd_name == "WATCH") {
      is_important_cmd = true;
    }
    
    if (is_important_cmd) {
      // 对于重要命令，我们保留原来的命令，并拒绝新命令
      // 恢复已移除的key映射
      std::vector<std::string> old_keys = old_cmd->current_key();
      for (const auto& key : old_keys) {
        key_map_[key] = it;
      }
      
      // 从待删除列表中移除该命令
      for (auto iter = to_remove.begin(); iter != to_remove.end(); ++iter) {
        if (*iter == it) {
          to_remove.erase(iter);
          break;
        }
      }
      
    }
  }
  
  // 删除非重要的冲突命令
  for (auto it : to_remove) {
    // 执行回调通知命令被取消
    if (it->second) {
      it->second(LogOffset(), pstd::Status::Busy("Command replaced by newer command with same key"));
    }
    
    // 从队列中移除
    pending_commands_.erase(it);
  }
}





void PikaCommandCollector::SetBatchSize(size_t batch_size) {
  batch_size_.store(batch_size);
  LOG(INFO) << "BatchSize set to " << batch_size;
}

void PikaCommandCollector::SetBatchMaxWaitTime(int batch_max_wait_time) {
  batch_max_wait_time_.store(batch_max_wait_time);
  LOG(INFO) << "BatchMaxWaitTime set to " << batch_max_wait_time << "ms";
}

std::pair<uint64_t, uint64_t> PikaCommandCollector::GetBatchStats() const {
  return {total_processed_.load(), total_batches_.load()};
}

std::optional<double> PikaCommandCollector::GetAverageBatchTime() const {
  uint64_t total_batches = total_batches_.load();
  if (total_batches == 0) {
    return std::nullopt;
  }
  return static_cast<double>(total_batch_time_ms_.load()) / total_batches;
}

size_t PikaCommandCollector::PendingCommands() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_commands_.size();
}



bool PikaCommandCollector::RetryBatch(
    const std::vector<std::shared_ptr<Cmd>>& commands,
    const std::vector<CommandCallback>& callbacks,
    int priority) {
  
  if (commands.empty() || callbacks.empty() || commands.size() != callbacks.size()) {
    LOG(WARNING) << "RetryBatch: Invalid input parameters, commands size: " << commands.size() 
                << ", callbacks size: " << callbacks.size();
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  
  LOG(INFO) << "RetryBatch: Retrying " << commands.size() << " commands with priority " << priority;
  
  // Add to retry queue with priority
  retry_queue_.emplace_front(std::make_tuple(priority, commands, callbacks));
  
  // Update statistics
  total_retries_.fetch_add(1);
  
  // Sort retry queue by priority (higher values first)
  std::sort(retry_queue_.begin(), retry_queue_.end(), 
           [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });
  
  // Process highest priority retry immediately if possible
  if (priority > 50 && pending_commands_.empty()) {
    // Process retry queue directly
    auto& [pri, cmds, cbs] = retry_queue_.front();
    
    // Add each command to the queue
    for (size_t i = 0; i < cmds.size(); ++i) {
      if (cmds[i]) {
        // Handle key conflicts
        HandleConflict(cmds[i]);
        
        // Add to queue front for priority processing
        pending_commands_.push_front(std::make_pair(cmds[i], cbs[i]));
        
        // Update key mapping
        std::vector<std::string> keys = cmds[i]->current_key();
        for (const auto& key : keys) {
          key_map_[key] = pending_commands_.begin();
        }
        
        LOG(INFO) << "RetryBatch: Immediately requeued command " << i << ": " << cmds[i]->name();
      }
    }
    
    // Remove from retry queue
    retry_queue_.pop_front();
    
    // Trigger immediate flush
    FlushCommands(true);
  }
  
  return true;
}