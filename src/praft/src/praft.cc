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
#include "binlog.pb.h"
#include "storage/storage.h"
#include "storage/batch.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaServer> g_pika_server;

namespace pika_raft {

// PikaStateMachine implementation
PikaStateMachine::PikaStateMachine() 
    : applied_index_(0), leader_term_(-1) {
}

void PikaStateMachine::on_apply(braft::Iterator& iter) {
  for (; iter.valid(); iter.next()) {
    // Use brpc::ClosureGuard instead of braft::AsyncClosureGuard for manual control
    auto done = iter.done();
    brpc::ClosureGuard done_guard(done);
    
    int64_t index = iter.index();
    butil::IOBuf log_data = iter.data();
    std::string log_str = log_data.to_string();
    
    if (!g_pika_server || !g_pika_server->GetRaftManager()) {
      applied_index_.store(index, std::memory_order_relaxed);
      // Run closure asynchronously in bthread to avoid blocking on_apply
      if (done) {
        braft::run_closure_in_bthread(done_guard.release());
      }
      continue;
    }
    
    // 直接解析 Protobuf binlog
    pikiwidb::Binlog binlog;
    if (!binlog.ParseFromString(log_str)) {
      if (done) {
        done->status().set_error(EINVAL, "Failed to parse binlog");
      }
      applied_index_.store(index, std::memory_order_relaxed);
      if (done) {
        braft::run_closure_in_bthread(done_guard.release());
      }
      continue;
    }
    
    // 应用 binlog 到 storage 并获取结果
    rocksdb::Status apply_status = g_pika_server->GetRaftManager()->ApplyBinlogEntry(log_str);
    
    // 根据应用结果设置 closure 状态
    if (done) {
      if (apply_status.ok()) {
        done->status().set_error(0, "OK");
      } else {
        done->status().set_error(-1, "%s", apply_status.ToString().c_str());
        LOG(ERROR) << "Apply binlog failed: " << apply_status.ToString();
      }
    }
    
    applied_index_.store(index, std::memory_order_relaxed);
    
    // Run closure asynchronously in bthread to avoid blocking on_apply
    if (done) {
      braft::run_closure_in_bthread(done_guard.release());
    }
  }
}

void PikaStateMachine::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  
  // Save applied index to snapshot
  std::string snapshot_path = writer->get_path() + "/pika_snapshot";
  
  // TODO: Save Pika DB state to snapshot
  // For now, just save the applied index
  
  std::string meta_file = writer->get_path() + "/meta";
  std::ofstream out(meta_file);
  if (out.is_open()) {
    out << applied_index_.load(std::memory_order_relaxed);
    out.close();
    
    if (writer->add_file("meta") != 0) {
      done->status().set_error(EIO, "Failed to add meta file");
      return;
    }
  }
}

int PikaStateMachine::on_snapshot_load(braft::SnapshotReader* reader) {
  // Load applied index from snapshot
  std::string meta_file = reader->get_path() + "/meta";
  std::ifstream in(meta_file);
  if (in.is_open()) {
    int64_t index;
    in >> index;
    in.close();
    
    applied_index_.store(index, std::memory_order_relaxed);
    return 0;
  }
  
  return -1;
}

void PikaStateMachine::on_leader_start(int64_t term) {
  leader_term_.store(term, std::memory_order_relaxed);
}

void PikaStateMachine::on_leader_stop(const butil::Status& status) {
  leader_term_.store(-1, std::memory_order_relaxed);
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
  
  LOG(INFO) << "Raft node initialized for group: " << group_id_;
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
  
  std::vector<braft::PeerId> peers;
  if (IsLeader()) {
    butil::Status st = node_->list_peers(&peers);
    if (st.ok() && !peers.empty()) {
      oss << "Cluster Members: ";
      for (size_t i = 0; i < peers.size(); i++) {
        oss << peers[i].to_string();
        if (i < peers.size() - 1) {
          oss << ",";
        }
      }
      oss << "\n";
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
  
  // Auto-initialize cluster from config if raft-peers is set
  std::string raft_peers = g_pika_conf->raft_peers();
  if (!raft_peers.empty()) {
    LOG(INFO) << "Auto-initializing Raft cluster from config, peers: " << raft_peers;
    
    // Parse peers
    std::vector<std::string> peer_list;
    std::stringstream ss(raft_peers);
    std::string peer;
    while (std::getline(ss, peer, ',')) {
      // Trim whitespace
      peer.erase(0, peer.find_first_not_of(" \t"));
      peer.erase(peer.find_last_not_of(" \t") + 1);
      if (!peer.empty()) {
        peer_list.push_back(peer);
      }
    }
    
    if (!peer_list.empty()) {
      // Check if cluster already initialized (avoid duplicate initialization after restart)
      auto existing_node = GetRaftNode("db0");
      if (existing_node) {
        LOG(INFO) << "Raft cluster already initialized, skipping auto-initialization";
      } else {
        // Initialize cluster for default db
        pstd::Status status = InitCluster("db0", peer_list);
        if (!status.ok()) {
          LOG(WARNING) << "Failed to auto-initialize Raft cluster: " << status.ToString();
          LOG(WARNING) << "You may need to manually run RAFT.CLUSTER INIT command";
        } else {
          LOG(INFO) << "Raft cluster auto-initialized successfully with " 
                    << peer_list.size() << " peers";
        }
      }
    }
  } else {
    LOG(INFO) << "No raft-peers configured, cluster initialization skipped";
    LOG(INFO) << "Use RAFT.CLUSTER INIT command to initialize cluster manually";
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

pstd::Status RaftManager::JoinCluster(const std::string& db_name, 
                                      const std::string& leader_addr) {
  // Parse leader address
  braft::PeerId leader_id = ParsePeerId(leader_addr);
  if (leader_id.is_empty()) {
    return pstd::Status::Corruption("Invalid leader address: " + leader_addr);
  }
  
  // For joining, we create a node with empty initial peers
  // The node will be added through the AddNode command from the leader
  std::vector<braft::PeerId> empty_peers;
  return CreateRaftNode(db_name, empty_peers);
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
  
  // Create peer ID for this node
  // Use localhost since Pika doesn't expose a host() method
  std::string addr = "127.0.0.1:" + std::to_string(g_pika_conf->port() + 3000);
  braft::PeerId peer_id = ParsePeerId(addr);
  if (peer_id.is_empty()) {
    return pstd::Status::Corruption("Failed to create peer ID");
  }
  
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
  
  // If promise is set, notify the waiting thread
  if (promise_) {
    if (status().ok()) {
      promise_->set_value(rocksdb::Status::OK());
    } else {
      promise_->set_value(rocksdb::Status::IOError(status().error_str()));
    }
    return;
  }
  
  // Legacy path for non-promise closures
  if (!status().ok()) {
    return;
  }
}

void RaftManager::AppendLog(const std::string& db_name, 
                            const ::pikiwidb::Binlog& log, 
                            std::promise<rocksdb::Status>&& promise) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    LOG(ERROR) << "Raft node not found for DB: " << db_name;
    promise.set_value(rocksdb::Status::NotFound("Raft node not found"));
    return;
  }
  
  if (!node->IsLeader()) {
    braft::PeerId leader = node->GetLeaderId();
    LOG(WARNING) << "Current node is not leader for DB: " << db_name 
                 << ", leader: " << leader.to_string();
    promise.set_value(rocksdb::Status::Incomplete("Not leader"));
    return;
  }
  
  // 创建 WriteDoneClosure 并传递 promise
  auto* done = new WriteDoneClosure();
  done->SetPromise(std::make_shared<std::promise<rocksdb::Status>>(std::move(promise)));
  
  // 序列化 binlog
  butil::IOBuf data;
  butil::IOBufAsZeroCopyOutputStream wrapper(&data);
  if (!log.SerializeToZeroCopyStream(&wrapper)) {
    done->status().set_error(-1, "Failed to serialize binlog");
    done->Run();
    return;
  }
  
  // 创建 Raft 任务
  braft::Task task;
  task.data = &data;
  task.done = done;
  
  // 提交到 Raft
  node->GetRaftNode()->apply(task);
}

// Apply binlog entry to storage (called from on_apply)
rocksdb::Status RaftManager::ApplyBinlogEntry(const std::string& binlog_data) {
  // 解析 binlog
  pikiwidb::Binlog binlog;
  if (!binlog.ParseFromString(binlog_data)) {
    return rocksdb::Status::Corruption("Failed to parse binlog");
  }
  
  // 从 binlog 中提取 db_name（假设默认是 db0，后续可以从 binlog 中读取）
  std::string db_name = "db0";
  
  // 从 PikaServer 获取对应的 DB 和 Storage
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
  
  // 调用 Storage::OnBinlogWrite() 应用 binlog
  // 注意：log_index 暂时传 0，后续可以从外部传入
  auto status = storage->OnBinlogWrite(binlog, 0);
  
  if (!status.ok()) {
    LOG(ERROR) << "Failed to apply binlog to " << db_name << ": " << status.ToString();
  }
  
  return status;
}

}  // namespace pika_raft

