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
#include "include/pika_command.h"
#include "include/pika_conf.h"
#include "include/pika_server.h"
#include "include/pika_cmd_table_manager.h"
#include "net/include/redis_conn.h"
#include "binlog.pb.h"
#include "storage/storage.h"
#include "storage/batch.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaServer> g_pika_server;
extern std::unique_ptr<PikaCmdTableManager> g_pika_cmd_table_manager;

// Thread-local flag to indicate if we're in on_apply context
namespace pika_raft {
thread_local bool g_in_raft_apply = false;

// RaftLogEntry serialization (simple format for now)
std::string RaftLogEntry::SerializeAsString() const {
  std::ostringstream oss;
  oss << cmd_name << "|" << db_name << "|" << timestamp << "|";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0) oss << ",";
    oss << args[i];
  }
  return oss.str();
}

bool RaftLogEntry::ParseFromString(const std::string& data) {
  size_t pos1 = data.find('|');
  if (pos1 == std::string::npos) return false;
  
  size_t pos2 = data.find('|', pos1 + 1);
  if (pos2 == std::string::npos) return false;
  
  size_t pos3 = data.find('|', pos2 + 1);
  if (pos3 == std::string::npos) return false;
  
  cmd_name = data.substr(0, pos1);
  db_name = data.substr(pos1 + 1, pos2 - pos1 - 1);
  
  try {
    timestamp = std::stoll(data.substr(pos2 + 1, pos3 - pos2 - 1));
  } catch (...) {
    return false;
  }
  
  std::string args_str = data.substr(pos3 + 1);
  if (!args_str.empty()) {
    size_t start = 0;
    size_t comma_pos;
    while ((comma_pos = args_str.find(',', start)) != std::string::npos) {
      args.push_back(args_str.substr(start, comma_pos - start));
      start = comma_pos + 1;
    }
    args.push_back(args_str.substr(start));
  }
  
  return true;
}

// PikaStateMachine implementation
PikaStateMachine::PikaStateMachine() 
    : applied_index_(0), leader_term_(-1) {
}

void PikaStateMachine::on_apply(braft::Iterator& iter) {
  for (; iter.valid(); iter.next()) {
    braft::AsyncClosureGuard closure_guard(iter.done());
    
    int64_t index = iter.index();
    butil::IOBuf log_data = iter.data();
    std::string log_str = log_data.to_string();
    
    if (!g_pika_server || !g_pika_server->GetRaftManager()) {
      applied_index_.store(index, std::memory_order_relaxed);
      continue;
    }
    
    // Detect log format: Redis protocol (Plan A) or Protobuf binlog (Plan B)
    // Redis protocol contains '|' separator (db_name|redis_proto)
    // Protobuf binlog is binary format (unlikely to contain '|' at the start)
    bool is_redis_protocol = (log_str.find('|') != std::string::npos);
    
    if (is_redis_protocol) {
      // Plan A: Redis protocol format
      // Extract db_name from log data
      // TODO: For now, use db0 as default. Need to encode db_name in log.
      // Could prepend db_name to Redis protocol: "<db_name>|*3\r\n..."
      std::string db_name = "db0";
      
      // Parse db_name if prepended
      size_t pipe_pos = log_str.find('|');
      if (pipe_pos != std::string::npos && pipe_pos < 20) {  // db_name should be short
        db_name = log_str.substr(0, pipe_pos);
        log_str = log_str.substr(pipe_pos + 1);  // Remove db_name prefix
      }
      
      rocksdb::Status status = g_pika_server->GetRaftManager()->ApplyCommandFromRedisProtocol(
          log_str, db_name);
      
      if (iter.done()) {
        if (status.ok()) {
          iter.done()->status().set_error(0, "OK");
        } else {
          iter.done()->status().set_error(-1, "%s", status.ToString().c_str());
        }
      }
    } else {
      // Plan B: Protobuf binlog format (legacy)
      pikiwidb::Binlog binlog;
      if (!binlog.ParseFromString(log_str)) {
        if (iter.done()) {
          iter.done()->status().set_error(EINVAL, "Failed to parse binlog");
        }
        applied_index_.store(index, std::memory_order_relaxed);
        continue;
      }
      
      g_pika_server->GetRaftManager()->ApplyBinlogEntry(log_str);
    }
    
    applied_index_.store(index, std::memory_order_relaxed);
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

pstd::Status PikaRaftNode::Apply(const RaftLogEntry& entry) {
  if (!node_) {
    return pstd::Status::Corruption("Raft node not initialized");
  }
  
  if (!IsLeader()) {
    braft::PeerId leader = GetLeaderId();
    std::string error_msg = "Not leader, current leader: " + leader.to_string();
    return pstd::Status::Corruption(error_msg);
  }
  
  // Serialize the entry
  std::string data = entry.SerializeAsString();
  butil::IOBuf log;
  log.append(data);
  
  // Apply to Raft
  braft::Task task;
  task.data = &log;
  task.done = nullptr; // Synchronous for now
  
  node_->apply(task);
  
  
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

bool RaftManager::IsRaftEnabled(const std::string& db_name) const {
  std::shared_lock lock(nodes_mutex_);
  return raft_nodes_.find(db_name) != raft_nodes_.end();
}

pstd::Status RaftManager::ApplyCommand(const std::string& db_name, 
                                       const RaftLogEntry& entry) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    return pstd::Status::Corruption("Raft node not found for DB: " + db_name);
  }
  
  return node->Apply(entry);
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
WriteDoneClosure::WriteDoneClosure(std::shared_ptr<Cmd> cmd, 
                                   std::shared_ptr<net::RedisConn> conn)
    : cmd_(std::move(cmd)), conn_(std::move(conn)) {
}

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

// ApplyBinlog implementation
pstd::Status RaftManager::ApplyBinlog(const std::string& db_name,
                                      const std::string& binlog_data,
                                      WriteDoneClosure* done) {
  auto node = GetRaftNode(db_name);
  if (!node) {
    if (done) {
      delete done;
    }
    return pstd::Status::Corruption("Raft node not found for DB: " + db_name);
  }
  
  if (!node->IsLeader()) {
    braft::PeerId leader = node->GetLeaderId();
    if (done) {
      delete done;
    }
    std::string error_msg = "Not leader, current leader: " + leader.to_string();
    return pstd::Status::Corruption(error_msg);
  }
  
  // Create Raft task
  butil::IOBuf log;
  log.append(binlog_data);
  
  braft::Task task;
  task.data = &log;
  task.done = done;  // Async callback
  
  node->GetRaftNode()->apply(task);
  return pstd::Status::OK();
}

// Apply binlog entry to storage (called from on_apply)
void RaftManager::ApplyBinlogEntry(const std::string& binlog_data) {
  if (!storage_) {
    return;
  }
  
  // 解析 binlog
  pikiwidb::Binlog binlog;
  if (!binlog.ParseFromString(binlog_data)) {
    return;
  }
  
  // 临时禁用 binlog 回调以避免循环
  auto old_callback = storage_->GetBinlogCallback();
  storage_->SetBinlogWriteCallback(nullptr);
  
  // 创建 RocksBatch 直接写入（不触发 binlog）
  auto batch = std::make_unique<storage::RocksBatch>(
    storage_->GetStringsDB(),
    storage_->GetHashesDB(),
    storage_->GetListsDB(),
    storage_->GetSetsDB(),
    storage_->GetZSetsDB(),
    storage_->GetStreamsDB()
  );
  
  // 应用所有条目到 batch
  for (const auto& entry : binlog.entries()) {
    storage::DataType dtype = storage::ProtoToStorageDataType(entry.data_type());
    
    switch (entry.op_type()) {
      case pikiwidb::OperateType::kPut:
        batch->Put(dtype,
                   rocksdb::Slice(entry.key()),
                   rocksdb::Slice(entry.value()));
        break;
        
      case pikiwidb::OperateType::kDelete:
        batch->Delete(dtype, rocksdb::Slice(entry.key()));
        break;
        
      default:
        break;
    }
  }
  
  // 提交批量写入
  auto status = batch->Commit();
  
  // 恢复 binlog 回调
  storage_->SetBinlogWriteCallback(old_callback);
}

// Submit command with promise (for Plan A)
pstd::Status RaftManager::SubmitCommandWithPromise(
    const std::string& db_name,
    const std::string& log_data,
    std::promise<rocksdb::Status>&& promise) {
  
  auto node = GetRaftNode(db_name);
  if (!node) {
    promise.set_value(rocksdb::Status::NotFound("Raft node not found"));
    return pstd::Status::NotFound("Raft node not found for db: " + db_name);
  }
  
  if (!node->IsLeader()) {
    braft::PeerId leader = node->GetLeaderId();
    promise.set_value(rocksdb::Status::Aborted("Not leader"));
    std::string error_msg = "Not leader, current leader: " + leader.to_string();
    return pstd::Status::Corruption(error_msg);
  }
  
  // Create closure with promise
  auto* closure = new WriteDoneClosure(nullptr, nullptr);
  closure->SetPromise(std::make_shared<std::promise<rocksdb::Status>>(
      std::move(promise)));
  
  // Create braft::Task
  braft::Task task;
  butil::IOBuf buf;
  buf.append(log_data);
  task.data = &buf;
  task.done = closure;
  
  // Submit to Raft
  node->GetRaftNode()->apply(task);
  
  return pstd::Status::OK();
}

// Apply command from Redis protocol (called in on_apply)
rocksdb::Status RaftManager::ApplyCommandFromRedisProtocol(
    const std::string& redis_proto_data,
    const std::string& db_name) {
  
  // Get DB
  auto db = g_pika_server->GetDB(db_name);
  if (!db) {
    return rocksdb::Status::NotFound("DB not found");
  }
  
  // Parse Redis protocol to extract command and args
  std::vector<std::string> argv;
  
  // Redis protocol format:
  // *<argc>\r\n$<len>\r\n<data>\r\n$<len>\r\n<data>\r\n...
  size_t pos = 0;
  if (redis_proto_data[pos] != '*') {
    return rocksdb::Status::Corruption("Invalid Redis protocol");
  }
  pos++;
  
  // Parse argc
  size_t newline_pos = redis_proto_data.find("\r\n", pos);
  if (newline_pos == std::string::npos) {
    return rocksdb::Status::Corruption("Invalid Redis protocol");
  }
  int argc = std::stoi(redis_proto_data.substr(pos, newline_pos - pos));
  pos = newline_pos + 2;
  
  // Parse each argument
  for (int i = 0; i < argc; i++) {
    if (pos >= redis_proto_data.size() || redis_proto_data[pos] != '$') {
      return rocksdb::Status::Corruption("Invalid Redis protocol");
    }
    pos++;
    
    // Parse argument length
    newline_pos = redis_proto_data.find("\r\n", pos);
    if (newline_pos == std::string::npos) {
      return rocksdb::Status::Corruption("Invalid Redis protocol");
    }
    int arg_len = std::stoi(redis_proto_data.substr(pos, newline_pos - pos));
    pos = newline_pos + 2;
    
    // Parse argument data
    if (pos + arg_len > redis_proto_data.size()) {
      return rocksdb::Status::Corruption("Invalid Redis protocol");
    }
    argv.push_back(redis_proto_data.substr(pos, arg_len));
    pos += arg_len + 2;  // Skip \r\n
  }
  
  if (argv.empty()) {
    return rocksdb::Status::Corruption("Empty command");
  }
  
  // Convert command name to lowercase (Pika command table uses lowercase)
  std::string cmd_name = argv[0];
  std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::tolower);
  
  // Set thread-local flag to indicate we're in on_apply context
  g_in_raft_apply = true;
  
  // Create command object
  std::shared_ptr<Cmd> cmd = g_pika_cmd_table_manager->GetCmd(cmd_name);
  if (!cmd) {
    g_in_raft_apply = false;
    return rocksdb::Status::NotSupported("Unknown command");
  }
  
  // Initialize and execute command
  cmd->Initial(argv, db_name);
  
  // Execute the full command flow. DoBinlog() will be called but will skip
  // because g_in_raft_apply is true.
  cmd->Execute();
  
  // Clear thread-local flag
  g_in_raft_apply = false;
  
  rocksdb::Status result;
  if (cmd->res().ok()) {
    result = rocksdb::Status::OK();
  } else {
    result = rocksdb::Status::IOError(cmd->res().message());
  }
  
  return result;
}

}  // namespace pika_raft

