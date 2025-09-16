// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <memory>
#include <vector>
#include <chrono>
#include "include/pika_command_collector.h"
#include "include/pika_rm.h"
#include "include/pika_conf.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaReplicaManager> g_pika_rm;

PikaCommandCollector::PikaCommandCollector(ConsensusCoordinator* coordinator, int batch_max_wait_time)
    : coordinator_(coordinator), batch_max_wait_time_(batch_max_wait_time) {
  LOG(INFO) << "PikaCommandCollector started for DB: " << coordinator_->db_name()
            << " with batch_max_wait_time: " << batch_max_wait_time << "ms";
}

PikaCommandCollector::PikaCommandCollector(std::shared_ptr<ConsensusCoordinator> coordinator, int batch_max_wait_time)
    : coordinator_(coordinator.get()), batch_max_wait_time_(batch_max_wait_time) {
  LOG(INFO) << "PikaCommandCollector started for DB: " << coordinator_->db_name()
            << " with batch_max_wait_time: " << batch_max_wait_time << "ms";
}

PikaCommandCollector::~PikaCommandCollector() {
  LOG(INFO) << "PikaCommandCollector stopped, processed " << total_processed_.load()
            << " commands, " << total_batches_.load() << " batches";
}

bool PikaCommandCollector::AddCommand(std::shared_ptr<Cmd> cmd_ptr, CommandCallback callback) {
  if (!cmd_ptr || !cmd_ptr->is_write()) {
    LOG(WARNING) << "Attempt to add non-write command to CommandCollector";
    return false;
  }

  // Create a single-command batch directly
  std::vector<std::shared_ptr<Cmd>> commands = {cmd_ptr};
  std::vector<CommandCallback> callbacks = {std::move(callback)};
  
  std::string db_name = cmd_ptr->db_name().empty() ? g_pika_conf->default_db() : cmd_ptr->db_name();
  auto command_batch = std::make_shared<CommandBatch>(commands, callbacks, db_name);
  
  // Enqueue the batch directly to PikaReplicaManager
  g_pika_rm->EnqueueCommandBatch(command_batch);
  
  // Update statistics
  total_processed_.fetch_add(1);
  total_batches_.fetch_add(1);
  
  //LOG(INFO) << "Added single command " << cmd_ptr->name() << " to CommandQueue";
  return true;
}

void PikaCommandCollector::SetBatchMaxWaitTime(int batch_max_wait_time) {
  batch_max_wait_time_.store(batch_max_wait_time);
  LOG(INFO) << "BatchMaxWaitTime set to " << batch_max_wait_time << "ms";
}

std::pair<uint64_t, uint64_t> PikaCommandCollector::GetBatchStats() const {
  return {total_processed_.load(), total_batches_.load()};
}