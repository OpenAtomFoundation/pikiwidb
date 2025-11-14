// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "praft/praft.h"

#include <glog/logging.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "braft/configuration.h"
#include "braft/repeated_timer_task.h"
#include "brpc/server.h"
#include "brpc/closure_guard.h"
#include "include/pika_conf.h"
#include "include/pika_server.h"
#include "include/pika_command.h"
#include "include/pika_client_conn.h"
#include "binlog.pb.h"
#include "storage/storage.h"
#include "storage/batch.h"
#include "pstd/include/env.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaServer> g_pika_server;

namespace pika_raft {

// PikaStateMachine implementation
PikaStateMachine::PikaStateMachine() {} 

void PikaStateMachine::on_apply(braft::Iterator& iter) {
  for (; iter.valid(); iter.next()) {
    auto done = iter.done();
    brpc::ClosureGuard done_guard(done);
    
    int64_t index = iter.index();
    
    if (!g_pika_server || !g_pika_server->GetRaftManager()) {
      // Run closure asynchronously in bthread to avoid blocking on_apply
      if (done) {
        braft::run_closure_in_bthread(done_guard.release());
      }
      continue;
    }
    
    pikiwidb::Binlog binlog;
    butil::IOBufAsZeroCopyInputStream wrapper(iter.data());
    if (!binlog.ParseFromZeroCopyStream(&wrapper)) {
      if (done) {
        done->status().set_error(EINVAL, "Failed to parse binlog");
      }
      if (done) {
        braft::run_closure_in_bthread(done_guard.release());
      }
      continue;
    }
    
    // Apply binlog with log index for tracking
    rocksdb::Status apply_status = g_pika_server->GetRaftManager()->ApplyBinlogEntry(binlog, index);
    
    if (done) {
      if (apply_status.ok()) {
        done->status().set_error(0, "OK");
      } else {
        done->status().set_error(-1, "%s", apply_status.ToString().c_str());
        LOG(ERROR) << "Apply binlog failed: " << apply_status.ToString();
      }
    }
    
    
    // Run closure asynchronously in bthread to avoid blocking on_apply
    if (done) {
      braft::run_closure_in_bthread(done_guard.release());
    }
  }
}

void PikaStateMachine::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {
  brpc::ClosureGuard done_guard(done);
}

int PikaStateMachine::on_snapshot_load(braft::SnapshotReader* reader) {
  std::string meta_file = reader->get_path() + "/meta";
  std::ifstream in(meta_file);
  if (in.is_open()) {
    int64_t index;
    in >> index;
    in.close();
    
    return 0;
  }
  
  return -1;
}

void PikaStateMachine::on_leader_start(int64_t term) {
}

void PikaStateMachine::on_leader_stop(const butil::Status& status) {
}

void PikaStateMachine::on_error(const ::braft::Error& e) {
  // Error occurred
}

void PikaStateMachine::on_configuration_committed(const ::braft::Configuration& conf) {
  // Configuration committed
}

void PikaStateMachine::on_start_following(const ::braft::LeaderChangeContext& ctx) {
  // Start following
}

void PikaStateMachine::on_stop_following(const ::braft::LeaderChangeContext& ctx) {
  // Stop following
}

// PikaRaftNode implementation
PikaRaftNode::PikaRaftNode(const std::string& group_id, const braft::PeerId& peer_id)
    : group_id_(group_id), peer_id_(peer_id) {
  // Setup Raft data directories
  raft_data_dir_ = g_pika_conf->db_path() + "/raft/" + group_id;
  raft_log_uri_ = "local://" + raft_data_dir_ + "/log";
  raft_meta_uri_ = "local://" + raft_data_dir_ + "/raft_meta";
  raft_snapshot_uri_ = "local://" + raft_data_dir_ + "/snapshot";
}

PikaRaftNode::~PikaRaftNode() {
  Shutdown();
}

pstd::Status PikaRaftNode::Init(const std::vector<braft::PeerId>& peers) {
  // Create state machine
  state_machine_ = std::make_unique<PikaStateMachine>();
  
  // Create and start brpc server for Raft RPC
  server_ = std::make_unique<brpc::Server>();
  
  // Add Raft service to brpc server
  if (braft::add_service(server_.get(), peer_id_.addr) != 0) {
    LOG(ERROR) << "Failed to add Raft service to brpc server";
    return pstd::Status::Corruption("Failed to add Raft service");
  }
  
  // Start brpc server
  if (server_->Start(peer_id_.addr.port, nullptr) != 0) {
    LOG(ERROR) << "Failed to start brpc server on " << peer_id_.addr;
    return pstd::Status::Corruption("Failed to start brpc server");
  }
  
  LOG(INFO) << "brpc server started on " << peer_id_.addr;
  
  // Setup Raft node options
  braft::NodeOptions node_options;
  
  // Set initial configuration
  for (const auto& peer : peers) {
    node_options.initial_conf.add_peer(peer);
  }
  
  // Set file system paths
  node_options.log_uri = raft_log_uri_;
  node_options.raft_meta_uri = raft_meta_uri_;
  node_options.snapshot_uri = raft_snapshot_uri_;
  
  // Set state machine
  node_options.fsm = state_machine_.get();
  
  // Set election timeout
  node_options.election_timeout_ms = g_pika_conf->raft_election_timeout_ms();
  
  // Set snapshot interval
  node_options.snapshot_interval_s = g_pika_conf->raft_snapshot_interval_s();
  
  // Create and initialize Raft node
  node_ = std::make_unique<braft::Node>(group_id_, peer_id_);
  
  if (node_->init(node_options) != 0) {
    LOG(ERROR) << "Failed to init Raft node";
    return pstd::Status::Corruption("Failed to init Raft node");
  }
  
  return pstd::Status::OK();
}

pstd::Status PikaRaftNode::Start() {
  if (!node_) {
    return pstd::Status::Corruption("Raft node not initialized");
  }
  
  return pstd::Status::OK();
}

void PikaRaftNode::Shutdown() {
  if (node_) {
    node_->shutdown(nullptr);
    node_->join();
    node_.reset();
  }
  
  if (server_) {
    server_->Stop(0);
    server_->Join();
    server_.reset();
  }
}

bool PikaRaftNode::IsLeader() const {
  if (!node_) return false;
  return node_->is_leader();
}

braft::PeerId PikaRaftNode::GetLeaderId() {
  if (!node_) return braft::PeerId();
  return node_->leader_id();
}

pstd::Status PikaRaftNode::AddPeer(const braft::PeerId& peer) {
  if (!node_) {
    return pstd::Status::Corruption("Raft node not initialized");
  }
  
  // Check if current node is leader
  // Member changes must be initiated by the leader
  if (!IsLeader()) {
    braft::PeerId leader = GetLeaderId();
    std::string error_msg = "Not leader. Current leader is: " + leader.to_string();
    LOG(WARNING) << "AddPeer failed: " << error_msg;
    return pstd::Status::Corruption(error_msg);
  }
  
  braft::SynchronizedClosure done;
  node_->add_peer(peer, &done);
  done.wait();
  
  if (!done.status().ok()) {
    return pstd::Status::Corruption("Failed to add peer: " + done.status().error_str());
  }
  
  return pstd::Status::OK();
}

pstd::Status PikaRaftNode::RemovePeer(const braft::PeerId& peer) {
  if (!node_) {
    return pstd::Status::Corruption("Raft node not initialized");
  }
  
  // Check if current node is leader
  // Member changes must be initiated by the leader
  if (!IsLeader()) {
    braft::PeerId leader = GetLeaderId();
    std::string error_msg = "Not leader. Current leader is: " + leader.to_string();
    LOG(WARNING) << "RemovePeer failed: " << error_msg;
    return pstd::Status::Corruption(error_msg);
  }
  
  braft::SynchronizedClosure done;
  node_->remove_peer(peer, &done);
  done.wait();
  
  if (!done.status().ok()) {
    return pstd::Status::Corruption("Failed to remove peer: " + done.status().error_str());
  }
  
  return pstd::Status::OK();
}

void PikaRaftNode::GetStatus(std::string* status_str) {
  if (!node_) {
    *status_str = "Raft node not initialized";
    return;
  }
  
  braft::NodeStatus status;
  node_->get_status(&status);
  
  std::ostringstream oss;
  oss << "Group: " << group_id_ << "\n";
  oss << "PeerId: " << peer_id_.to_string() << "\n";
  oss << "State: " << (IsLeader() ? "LEADER" : "FOLLOWER") << "\n";
  oss << "Leader: " << status.leader_id.to_string() << "\n";
  oss << "Term: " << status.term << "\n";
  
  // Try to list cluster members
  std::vector<braft::PeerId> peers;
  butil::Status st = node_->list_peers(&peers);
  
  if (st.ok() && !peers.empty()) {
    oss << "Cluster Members (" << peers.size() << "): ";
    for (size_t i = 0; i < peers.size(); i++) {
      oss << peers[i].to_string();
      if (i < peers.size() - 1) {
        oss << ", ";
      }
    }
    oss << "\n";
  } else {
    // For Follower nodes, list_peers() may not work, suggest querying Leader
    if (!IsLeader()) {
      oss << "Cluster Members: Query leader at " << status.leader_id.to_string() 
          << " for full member list\n";
    } else {
      oss << "Cluster Members: Unable to retrieve\n";
    }
  }
  
  oss << "Committed Index: " << status.committed_index << "\n";
  oss << "Known Applied Index: " << status.known_applied_index << "\n";
  oss << "Pending Index: " << status.pending_index << "\n";
  oss << "Pending Queue Size: " << status.pending_queue_size << "\n";
  oss << "Applying Index: " << status.applying_index << "\n";
  oss << "First Index: " << status.first_index << "\n";
  oss << "Last Index: " << status.last_index << "\n";
  
  *status_str = oss.str();
}

// RaftManager implementation
RaftManager::RaftManager()
    : initialized_(false),
      running_(false),
      election_timeout_ms_(1000),
      snapshot_interval_s_(3600) {
}

RaftManager::~RaftManager() {
  Shutdown();
}

pstd::Status RaftManager::Init() {
  if (initialized_.load()) {
    return pstd::Status::OK();
  }
  
  // Load configuration
  election_timeout_ms_ = g_pika_conf->raft_election_timeout_ms();
  snapshot_interval_s_ = g_pika_conf->raft_snapshot_interval_s();
  group_id_ = g_pika_conf->raft_group_id();
  
  LOG(INFO) << "Initializing Raft manager with group_id: " << group_id_
            << ", election_timeout: " << election_timeout_ms_ << "ms"
            << ", snapshot_interval: " << snapshot_interval_s_ << "s";
  
  // Check if Raft metadata directory exists (node was previously in a cluster)
  std::string raft_meta_dir = g_pika_conf->db_path() + "/raft/" + group_id_ + "_db0/raft_meta";
  bool raft_meta_exists = pstd::FileExists(raft_meta_dir);
  
  if (raft_meta_exists) {
    // Node was previously in a cluster, restore from persisted metadata
    LOG(INFO) << "Raft metadata directory exists, node was previously in cluster. Restoring from persisted configuration...";
    
    // When raft_meta exists, braft will ignore initial_conf and load configuration from raft_meta
    // We pass an empty peer list as it will be ignored anyway
    std::vector<std::string> empty_peer_list;
    pstd::Status status = InitCluster("db0", empty_peer_list);
    if (!status.ok()) {
      LOG(ERROR) << "Failed to restore Raft node from metadata: " << status.ToString();
    } else {
      LOG(INFO) << "Raft node restored successfully from persisted configuration";
    }
  } else {
    // First time startup - no raft_meta found
    // Do not auto-initialize, require manual initialization via RAFT.CLUSTER INIT command
    LOG(INFO) << "No existing Raft metadata found.";
    LOG(INFO) << "This is the first time starting Raft on this node.";
    LOG(INFO) << "Please initialize the cluster manually using: RAFT.CLUSTER INIT [peers]";
    LOG(INFO) << "  - For single-node cluster: RAFT.CLUSTER INIT";
    LOG(INFO) << "  - For multi-node cluster: RAFT.CLUSTER INIT <peer1>,<peer2>,...";
  }
  
  initialized_.store(true);
  return pstd::Status::OK();
}

pstd::Status RaftManager::Start() {
  if (!initialized_.load()) {
    return pstd::Status::Corruption("RaftManager not initialized");
  }
  
  if (running_.load()) {
    return pstd::Status::OK();
  }
  
  LOG(INFO) << "Starting Raft manager";
  
  // Start all Raft nodes
  std::shared_lock lock(nodes_mutex_);
  for (auto& pair : raft_nodes_) {
    auto status = pair.second->Start();
    if (!status.ok()) {
      LOG(ERROR) << "Failed to start Raft node for DB: " << pair.first;
      return status;
    }
  }
  
  running_.store(true);
  LOG(INFO) << "Raft manager started successfully";
  return pstd::Status::OK();
}

void RaftManager::Shutdown() {
  if (!running_.load()) {
    return;
  }
  
  LOG(INFO) << "Shutting down Raft manager";
  
  // Shutdown all Raft nodes
  std::unique_lock lock(nodes_mutex_);
  for (auto& pair : raft_nodes_) {
    pair.second->Shutdown();
  }
  raft_nodes_.clear();
  
  running_.store(false);
  initialized_.store(false);
  LOG(INFO) << "Raft manager shutdown complete";
}

pstd::Status RaftManager::InitCluster(const std::string& db_name, 
                                      const std::vector<std::string>& peers) {
  std::vector<braft::PeerId> peer_ids;
  for (const auto& peer_str : peers) {
    braft::PeerId peer_id = ParsePeerId(peer_str);
    if (peer_id.is_empty()) {
      return pstd::Status::Corruption("Invalid peer address: " + peer_str);
    }
    peer_ids.push_back(peer_id);
  }
  
  return CreateRaftNode(db_name, peer_ids);
}


pstd::Status RaftManager::AddNode(const std::string& db_name, 
                                  const std::string& peer_addr) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    return pstd::Status::Corruption("Raft node not found for DB: " + db_name);
  }
  
  braft::PeerId peer_id = ParsePeerId(peer_addr);
  if (peer_id.is_empty()) {
    return pstd::Status::Corruption("Invalid peer address: " + peer_addr);
  }
  
  return node->AddPeer(peer_id);
}

pstd::Status RaftManager::RemoveNode(const std::string& db_name, 
                                     const std::string& peer_addr) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    return pstd::Status::Corruption("Raft node not found for DB: " + db_name);
  }
  
  braft::PeerId peer_id = ParsePeerId(peer_addr);
    
  if (peer_id.is_empty()) {
    return pstd::Status::Corruption("Invalid peer address: " + peer_addr);
  }
  
  return node->RemovePeer(peer_id);
}

pstd::Status RaftManager::GetClusterInfo(const std::string& db_name, 
                                         std::string* info) {
  auto node = GetRaftNode(db_name);

  if (!node) {
    return pstd::Status::Corruption("Raft node not found for DB: " + db_name);
  }
  
  node->GetStatus(info);
  return pstd::Status::OK();
}

std::shared_ptr<PikaRaftNode> RaftManager::GetRaftNode(const std::string& db_name) {
  std::shared_lock lock(nodes_mutex_);
  auto it = raft_nodes_.find(db_name);
  if (it != raft_nodes_.end()) {
    return it->second;
  }
  return nullptr;
}

pstd::Status RaftManager::CreateRaftNode(const std::string& db_name, 
                                         const std::vector<braft::PeerId>& peers) {
  std::unique_lock lock(nodes_mutex_);
  
  // Check if node already exists
  if (raft_nodes_.find(db_name) != raft_nodes_.end()) {
    return pstd::Status::Corruption("Raft node already exists for DB: " + db_name);
  }
  
  // Determine the Raft port for this node
  // Raft uses Pika port + 3000
  int raft_port = g_pika_conf->port() + 3000;
  
  // Find the peer address from the peers list that matches our Raft port
  // This allows the user to specify the exact address in RAFT.CLUSTER INIT command
  braft::PeerId peer_id;
  bool found = false;
  
  for (const auto& peer : peers) {
    if (peer.addr.port == raft_port) {
      peer_id = peer;
      found = true;
      LOG(INFO) << "Found matching peer address in cluster config: " << peer.to_string();
      break;
    }
  }
  
  // If no matching peer found, return error
  if (!found) {
    std::string error_msg = "No matching peer address found in cluster config for Raft port " + 
                           std::to_string(raft_port) + 
                           ". Please include this node's address (with port " + 
                           std::to_string(raft_port) + 
                           ") in the RAFT.CLUSTER INIT command. " +
                           "Example: RAFT.CLUSTER INIT <ip1>:" + std::to_string(raft_port) + 
                           ",<ip2>:<port2>,...";
    LOG(ERROR) << error_msg;
    return pstd::Status::Corruption(error_msg);
  }
  
  LOG(INFO) << "Creating Raft node for DB: " << db_name << " with address: " << peer_id.to_string();
  
  // Create Raft node
  auto node = std::make_shared<PikaRaftNode>(group_id_ + "_" + db_name, peer_id);
  
  // Initialize node
  auto status = node->Init(peers);
  if (!status.ok()) {
    return status;
  }
  
  // Start node if manager is running
  if (running_.load()) {
    status = node->Start();
    if (!status.ok()) {
      return status;
    }
  }
  
  // Store node
  raft_nodes_[db_name] = node;
  
  LOG(INFO) << "Created Raft node for DB: " << db_name;
  return pstd::Status::OK();
}

braft::PeerId RaftManager::ParsePeerId(const std::string& peer_str) {
  braft::PeerId peer_id;
  if (peer_id.parse(peer_str) != 0) {
    return braft::PeerId();
  }
  return peer_id;
}

// WriteDoneClosure implementation

void WriteDoneClosure::Run() {
  std::unique_ptr<WriteDoneClosure> self_guard(this);
  
  // If promise is set, notify the waiting thread (synchronous mode)
  if (promise_) {
    if (status().ok()) {
      promise_->set_value(rocksdb::Status::OK());
    } else {
      promise_->set_value(rocksdb::Status::IOError(status().error_str()));
    }
    return;
  }
  
  // If callback is set, call it (asynchronous mode for Leader)
  if (callback_) {
    rocksdb::Status s;
    if (status().ok()) {
      s = rocksdb::Status::OK();
    } else {
      s = rocksdb::Status::IOError(status().error_str());
    }
    // Call callback with status only (result is captured in lambda)
    callback_(s);
    return;
  }
  
  // Legacy path for closures without promise or callback
  if (!status().ok()) {
    LOG(WARNING) << "Raft operation failed: " << status().error_str();
  }
}

void RaftManager::AppendLog(const std::string& db_name, 
                            const ::pikiwidb::Binlog& log, 
                            std::promise<rocksdb::Status>&& promise,
                            storage::CommitCallback callback) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    LOG(ERROR) << "Raft node not found for DB: " << db_name;
    if (callback) {
      callback(rocksdb::Status::NotFound("Raft node not found"));
    } else {
      promise.set_value(rocksdb::Status::NotFound("Raft node not found"));
    }
    return;
  }
  
  if (!node->IsLeader()) {
    braft::PeerId leader = node->GetLeaderId();
    LOG(WARNING) << "Current node is not leader for DB: " << db_name 
                 << ", leader: " << leader.to_string();
    if (callback) {
      callback(rocksdb::Status::Incomplete("Not leader, leader is: " + leader.to_string()));
    } else {
      promise.set_value(rocksdb::Status::Incomplete("Not leader"));
    }
    return;
  }
  
  auto* done = new WriteDoneClosure();
  
  if (callback) {
    done->SetCallback(callback);
  } else {
    done->SetPromise(std::make_shared<std::promise<rocksdb::Status>>(std::move(promise)));
  }
  
  butil::IOBuf data;
  butil::IOBufAsZeroCopyOutputStream wrapper(&data);
  if (!log.SerializeToZeroCopyStream(&wrapper)) {
    done->status().set_error(-1, "Failed to serialize binlog");
    done->Run();
    return;
  }
  
  braft::Task task;
  task.data = &data;
  task.done = done;
  
  node->GetRaftNode()->apply(task);
}

rocksdb::Status RaftManager::ApplyBinlogEntry(const ::pikiwidb::Binlog& binlog, uint64_t log_index) {
  std::string db_name = "db0";
  
  auto db = g_pika_server->GetDB(db_name);
  if (!db) {
    LOG(ERROR) << "Failed to get DB: " << db_name;
    return rocksdb::Status::NotFound("DB not found: " + db_name);
  }
  
  auto storage = db->storage();
  if (!storage) {
    LOG(ERROR) << "Storage is null for DB: " << db_name;
    return rocksdb::Status::InvalidArgument("Storage is null");
  }
  
  // Pass log_index to storage layer for tracking
  auto status = storage->OnBinlogWrite(binlog, log_index);
  
  if (!status.ok()) {
    LOG(ERROR) << "Failed to apply binlog to " << db_name << " at log_index " << log_index 
               << ": " << status.ToString();
  }
  
  return status;
}

}  // namespace pika_raft
