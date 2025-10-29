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
    if (argv_.size() < 3) {
      res_.SetRes(CmdRes::kWrongNum, "RAFT.CLUSTER INIT requires peers argument");
      return;
    }
    // Parse peers from comma-separated list
    args_ = SplitString(argv_[2], ',');
    if (args_.empty()) {
      res_.SetRes(CmdRes::kInvalidParameter, "Invalid peers list");
      return;
    }
    // Optional db_name
    db_name_ = (argv_.size() >= 4) ? argv_[3] : "db0";
  } else if (op == "JOIN") {
    operation_ = Operation::JOIN;
    if (argv_.size() < 3) {
      res_.SetRes(CmdRes::kWrongNum, "RAFT.CLUSTER JOIN requires leader address");
      return;
    }
    args_.push_back(argv_[2]); // leader address
    db_name_ = (argv_.size() >= 4) ? argv_[3] : "db0";
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
        // Update raft-peers in config file and persist
        std::string peers_str = argv_[2];
        if (g_pika_conf->SetConfStr("raft-peers", peers_str)) {
          if (g_pika_conf->WriteBack()) {
            LOG(INFO) << "Updated raft-peers in config file: " << peers_str;
          } else {
            LOG(WARNING) << "Failed to write raft-peers to config file";
          }
        } else {
          LOG(WARNING) << "Failed to update raft-peers in config";
        }
        res_.AppendStringRaw("+OK\r\n");
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to initialize cluster: " + status.ToString());
      }
      break;
    }
    
    case Operation::JOIN: {
      LOG(INFO) << "Joining Raft cluster for DB: " << db_name_ 
                << " to leader: " << args_[0];
      status = raft_mgr->JoinCluster(db_name_, args_[0]);
      if (status.ok()) {
        res_.AppendStringRaw("+OK\r\n");
      } else {
        res_.SetRes(CmdRes::kErrOther, "Failed to join cluster: " + status.ToString());
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
        // Update raft-peers in config file
        std::string current_peers = g_pika_conf->raft_peers();
        if (current_peers.empty()) {
          current_peers = peer_addr_;
        } else if (current_peers.find(peer_addr_) == std::string::npos) {
          current_peers += "," + peer_addr_;
        }
        
        if (g_pika_conf->SetConfStr("raft-peers", current_peers)) {
          if (g_pika_conf->WriteBack()) {
            LOG(INFO) << "Updated raft-peers in config file: " << current_peers;
          } else {
            LOG(WARNING) << "Failed to write raft-peers to config file";
          }
        }
        
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
        // Update raft-peers in config file
        std::string current_peers = g_pika_conf->raft_peers();
        if (!current_peers.empty()) {
          // Parse and rebuild peer list without the removed peer
          std::vector<std::string> peer_list;
          std::stringstream ss(current_peers);
          std::string peer;
          while (std::getline(ss, peer, ',')) {
            peer.erase(0, peer.find_first_not_of(" \t"));
            peer.erase(peer.find_last_not_of(" \t") + 1);
            if (!peer.empty() && peer != peer_addr_) {
              peer_list.push_back(peer);
            }
          }
          
          // Rebuild peers string
          std::string new_peers;
          for (size_t i = 0; i < peer_list.size(); i++) {
            new_peers += peer_list[i];
            if (i < peer_list.size() - 1) {
              new_peers += ",";
            }
          }
          
          if (g_pika_conf->SetConfStr("raft-peers", new_peers)) {
            if (g_pika_conf->WriteBack()) {
              LOG(INFO) << "Updated raft-peers in config file: " << new_peers;
            } else {
              LOG(WARNING) << "Failed to write raft-peers to config file";
            }
          }
        }
        
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

// RaftConfigCmd implementation
void RaftConfigCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, name());
    return;
  }
  
  // Parse operation
  std::string op = argv_[1];
  std::transform(op.begin(), op.end(), op.begin(), ::toupper);
  
  if (op == "GET") {
    operation_ = Operation::GET;
    if (argv_.size() < 3) {
      res_.SetRes(CmdRes::kWrongNum, "RAFT.CONFIG GET requires config key");
      return;
    }
    config_key_ = argv_[2];
    db_name_ = (argv_.size() >= 4) ? argv_[3] : "db0";
  } else if (op == "SET") {
    operation_ = Operation::SET;
    if (argv_.size() < 4) {
      res_.SetRes(CmdRes::kWrongNum, "RAFT.CONFIG SET requires config key and value");
      return;
    }
    config_key_ = argv_[2];
    config_value_ = argv_[3];
    db_name_ = (argv_.size() >= 5) ? argv_[4] : "db0";
  } else {
    res_.SetRes(CmdRes::kInvalidParameter, "Unknown operation: " + op);
    return;
  }
}

void RaftConfigCmd::Do() {
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
  
  switch (operation_) {
    case Operation::GET: {
      std::string key_lower = config_key_;
      std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), ::tolower);
      
      std::string value;
      if (key_lower == "election-timeout-ms" || key_lower == "election_timeout_ms") {
        value = std::to_string(g_pika_conf->raft_election_timeout_ms());
      } else if (key_lower == "snapshot-interval-s" || key_lower == "snapshot_interval_s") {
        value = std::to_string(g_pika_conf->raft_snapshot_interval_s());
      } else if (key_lower == "group-id" || key_lower == "group_id") {
        value = g_pika_conf->raft_group_id();
      } else if (key_lower == "peers") {
        value = g_pika_conf->raft_peers();
      } else if (key_lower == "enabled") {
        value = g_pika_conf->raft_enabled() ? "yes" : "no";
      } else {
        res_.SetRes(CmdRes::kErrOther, "Unknown config key: " + config_key_);
        return;
      }
      
      res_.AppendStringLenUint64(value.size());
      res_.AppendContent(value);
      break;
    }
    
    case Operation::SET: {
      // Note: Runtime config changes would require rewriting config file
      // or storing in memory. For now, just return a message.
      LOG(INFO) << "Raft config SET requested: " << config_key_ << " = " << config_value_;
      res_.SetRes(CmdRes::kErrOther, 
                  "Runtime config modification not yet implemented. "
                  "Please modify pika.conf and restart.");
      break;
    }
    
    default:
      res_.SetRes(CmdRes::kErrOther, "Unknown operation");
      break;
  }
}

