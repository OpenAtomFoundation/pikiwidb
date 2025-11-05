// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_raft.h"

#include <glog/logging.h>
#include <algorithm>
#include <sstream>

// Include praft.h before pika_server.h to get complete RaftManager definition
#include "praft/praft.h"
#include "include/pika_conf.h"
#include "include/pika_server.h"
#include "pstd/include/pstd_string.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaServer> g_pika_server;

// Helper function to split string by delimiter
static std::vector<std::string> SplitString(const std::string& str, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    if (!item.empty()) {
      result.push_back(item);
    }
  }
  return result;
}

// RaftClusterCmd implementation
void RaftClusterCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, name());
    return;
  }
  
  // Parse operation
  std::string op = argv_[1];
  std::transform(op.begin(), op.end(), op.begin(), ::toupper);
  
  if (op == "INIT") {
    operation_ = Operation::INIT;
    
    if (argv_.size() >= 3 && !argv_[2].empty()) {
      // Peers specified - multi-node cluster initialization
      args_ = SplitString(argv_[2], ',');
      if (args_.empty()) {
        res_.SetRes(CmdRes::kInvalidParameter, "Invalid peers list");
        return;
      }
      db_name_ = (argv_.size() >= 4) ? argv_[3] : "db0";
    } else {
      // No peers specified - prepare node for being added to cluster
      // args_ remains empty, brpc server will start but no cluster config
      // The node will wait to be added via RAFT.NODE ADD from another cluster's leader
      db_name_ = (argv_.size() >= 3) ? argv_[2] : "db0";
      
      LOG(INFO) << "Preparing Raft node (no initial cluster config), waiting to be added to an existing cluster";
    }
  } else if (op == "INFO") {
    operation_ = Operation::INFO;
    db_name_ = (argv_.size() >= 3) ? argv_[2] : "db0";
  } else {
    res_.SetRes(CmdRes::kInvalidParameter, "Unknown operation: " + op);
    return;
  }
}

void RaftClusterCmd::Do() {
  // Check if Raft is enabled
  if (!g_pika_conf->raft_enabled()) {
    res_.SetRes(CmdRes::kErrOther, "Raft is not enabled in configuration");
    return;
  }
  
  auto raft_mgr = g_pika_server->GetRaftManager();
  if (!raft_mgr) {
    res_.SetRes(CmdRes::kErrOther, "Raft manager not initialized");
    return;
  }
  
  pstd::Status status;
  
  switch (operation_) {
    case Operation::INIT: {
      LOG(INFO) << "Initializing Raft cluster for DB: " << db_name_ 
                << " with peers: " << argv_[2];
      status = raft_mgr->InitCluster(db_name_, args_);
      if (status.ok()) {
        res_.AppendStringRaw("+OK\r\n");
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to initialize cluster: " + status.ToString());
      }
      break;
    }
    
    case Operation::INFO: {
      std::string info;
      status = raft_mgr->GetClusterInfo(db_name_, &info);
      if (status.ok()) {
        std::vector<std::string> lines;
        std::stringstream ss(info);
        std::string line;
        while (std::getline(ss, line)) {
          if (!line.empty()) {
            lines.push_back(line);
          }
        }
        
        res_.AppendArrayLen(lines.size());
        for (const auto& l : lines) {
          res_.AppendStringLen(l.size());
          res_.AppendContent(l);
        }
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to get cluster info: " + status.ToString());
      }
      break;
    }
    
    default:
      res_.SetRes(CmdRes::kErrOther, "Unknown operation");
      break;
  }
}

// RaftNodeCmd implementation
void RaftNodeCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, name());
    return;
  }
  
  // Parse operation
  std::string op = argv_[1];
  std::transform(op.begin(), op.end(), op.begin(), ::toupper);
  
  if (op == "ADD") {
    operation_ = Operation::ADD;
  } else if (op == "REMOVE") {
    operation_ = Operation::REMOVE;
  } else {
    res_.SetRes(CmdRes::kInvalidParameter, "Unknown operation: " + op);
    return;
  }
  
  if (argv_.size() < 3) {
    res_.SetRes(CmdRes::kWrongNum, "RAFT.NODE requires peer address");
    return;
  }
  
  peer_addr_ = argv_[2];
  db_name_ = (argv_.size() >= 4) ? argv_[3] : "db0";
}

void RaftNodeCmd::Do() {
  // Check if Raft is enabled
  if (!g_pika_conf->raft_enabled()) {
    res_.SetRes(CmdRes::kErrOther, "Raft is not enabled in configuration");
    return;
  }
  
  auto raft_mgr = g_pika_server->GetRaftManager();
  if (!raft_mgr) {
    res_.SetRes(CmdRes::kErrOther, "Raft manager not initialized");
    return;
  }
  
  pstd::Status status;
  
  switch (operation_) {
    case Operation::ADD: {
      LOG(INFO) << "Adding node to Raft cluster, DB: " << db_name_ 
                << ", peer: " << peer_addr_;
      status = raft_mgr->AddNode(db_name_, peer_addr_);
      if (status.ok()) {
        // Don't modify config file - braft manages cluster membership in raft_meta
        // The raft-peers in config file is only used for initial bootstrap
        LOG(INFO) << "Node added successfully to Raft cluster (managed by braft raft_meta)";
        
        res_.AppendStringRaw("+OK\r\n");
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to add node: " + status.ToString());
      }
      break;
    }
    
    case Operation::REMOVE: {
      LOG(INFO) << "Removing node from Raft cluster, DB: " << db_name_ 
                << ", peer: " << peer_addr_;
      status = raft_mgr->RemoveNode(db_name_, peer_addr_);
      if (status.ok()) {
        // Don't modify config file - braft manages cluster membership in raft_meta
        // The raft-peers in config file is only used for initial bootstrap
        LOG(INFO) << "Node removed successfully from Raft cluster (managed by braft raft_meta)";
        
        res_.AppendStringRaw("+OK\r\n");
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to remove node: " + status.ToString());
      }
      break;
    }
    
    default:
      res_.SetRes(CmdRes::kErrOther, "Unknown operation");
      break;
  }
}

