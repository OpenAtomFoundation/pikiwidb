// Copyright (c) 2018-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_cmd_table_manager.h"

#include <sys/syscall.h>
#include <unistd.h>

#include "include/acl.h"
#include "include/pika_conf.h"
#include "pstd/include/pstd_mutex.h"

extern std::unique_ptr<PikaConf> g_pika_conf;

void PikaCmdTableManager::ResetCommandCount() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::minutes(1)); 
    {
      std::lock_guard<std::mutex> lock(command_mutex_);
      slow_command_count_.clear();
      InitHistograms(); 
    }
  }
}

void PikaCmdTableManager::InitHistograms() {
  histograms_.clear();
  prometheus_registry_ = std::make_shared<prometheus::Registry>();
  histogram_family_ = &prometheus::BuildHistogram()
      .Name("pika_command_duration_seconds")
      .Help("Execution time of Pika commands in seconds")
      .Register(*prometheus_registry_);
}

PikaCmdTableManager::PikaCmdTableManager() {
  cmds_ = std::make_unique<CmdTable>();
  cmds_->reserve(300);
  InitHistograms();
  reset_thread_ = std::thread(&PikaCmdTableManager::ResetCommandCount, this);
}

void PikaCmdTableManager::InitCmdTable(void) {
  ::InitCmdTable(cmds_.get());
  for (const auto& cmd : *cmds_) {
    if (cmd.second->flag() & kCmdFlagsWrite) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::WRITE));
    }
    if (cmd.second->flag() & kCmdFlagsRead &&
        !(cmd.second->AclCategory() & static_cast<uint32_t>(AclCategory::SCRIPTING))) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::READ));
    }
    if (cmd.second->flag() & kCmdFlagsAdmin) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::ADMIN) |
                                 static_cast<uint32_t>(AclCategory::DANGEROUS));
    }
    if (cmd.second->flag() & kCmdFlagsPubSub) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::PUBSUB));
    }
    if (cmd.second->flag() & kCmdFlagsFast) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::FAST));
    }
    if (cmd.second->flag() & kCmdFlagsSlow) {
      cmd.second->AddAclCategory(static_cast<uint32_t>(AclCategory::SLOW));
    }
  }

  CommandStatistics statistics;
  for (auto& iter : *cmds_) {
    cmdstat_map_.emplace(iter.first, statistics);
    iter.second->SetCmdId(cmdId_++);
  }
}

void PikaCmdTableManager::RenameCommand(const std::string before, const std::string after) {
  auto it = cmds_->find(before);
  if (it != cmds_->end()) {
    if (after.length() > 0) {
      cmds_->insert(std::pair<std::string, std::unique_ptr<Cmd>>(after, std::move(it->second)));
    } else {
      LOG(ERROR) << "The value of rename-command is null";
    }
    cmds_->erase(it);
  }
}

prometheus::Histogram& PikaCmdTableManager::GetHistogram(const std::string& opt) {
  {
    std::shared_lock<std::shared_mutex> read_lock(histograms_mutex_);
    auto it = histograms_.find(opt);
    if (it != histograms_.end()) {
      return *(it->second);
    }
  }
  std::unique_lock<std::shared_mutex> write_lock(histograms_mutex_);
  auto& new_histogram = histogram_family_->Add(
    {{"command", opt}}, 
    prometheus::Histogram::BucketBoundaries{0.5, 1, 2, 3, 5, 7, 10, 15, 20, 30, 40, 50, 65, 75, 85, 100, 125, 140, 150, 160, 175, 185, 200, 300, 400, 500, 750, 1000, 2000, 5000, 10000}
  );
  histograms_[opt] = &new_histogram;
  return new_histogram;
}

prometheus::Family<prometheus::Histogram>* PikaCmdTableManager::GetHistograms() {
  return histogram_family_;
}
std::unordered_map<std::string, CommandStatistics>* PikaCmdTableManager::GetSlowCommandCount() {
  return &slow_command_count_;
}

std::unordered_map<std::string, CommandStatistics>* PikaCmdTableManager::GetCommandStatMap() {
  return &cmdstat_map_;
}

std::shared_ptr<Cmd> PikaCmdTableManager::GetCmd(const std::string& opt) {
  const std::string& internal_opt = opt;
  return NewCommand(internal_opt);
}

std::shared_ptr<Cmd> PikaCmdTableManager::NewCommand(const std::string& opt) {
  Cmd* cmd = GetCmdFromDB(opt, *cmds_);
  if (cmd) {
    return std::shared_ptr<Cmd>(cmd->Clone());
  }
  return nullptr;
}

CmdTable* PikaCmdTableManager::GetCmdTable() { return cmds_.get(); }

uint32_t PikaCmdTableManager::GetMaxCmdId() { return cmdId_; }

bool PikaCmdTableManager::CheckCurrentThreadDistributionMapExist(const std::thread::id& tid) {
  std::shared_lock l(map_protector_);
  return thread_distribution_map_.find(tid) != thread_distribution_map_.end();
}

void PikaCmdTableManager::InsertCurrentThreadDistributionMap() {
  auto tid = std::this_thread::get_id();
  std::unique_ptr<PikaDataDistribution> distribution = std::make_unique<HashModulo>();
  distribution->Init();
  std::lock_guard l(map_protector_);
  thread_distribution_map_.emplace(tid, std::move(distribution));
}

bool PikaCmdTableManager::CmdExist(const std::string& cmd) const { return cmds_->find(cmd) != cmds_->end(); }

std::vector<std::string> PikaCmdTableManager::GetAclCategoryCmdNames(uint32_t flag) {
  std::vector<std::string> result;
  for (const auto& item : (*cmds_)) {
    if (item.second->AclCategory() & flag) {
      result.emplace_back(item.first);
    }
  }
  return result;
}
