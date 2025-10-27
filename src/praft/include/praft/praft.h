// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PRAFT_PRAFT_H_
#define PRAFT_PRAFT_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <future>

#include "braft/raft.h"
#include "braft/storage.h"
#include "braft/util.h"
#include "pstd/include/pstd_mutex.h"
#include "pstd/include/pstd_status.h"
#include "rocksdb/status.h"
#include "batch_manager.h"

class PikaServer;

// Forward declarations
namespace storage {
class Storage;
}

namespace net {
class RedisConn;
}

class Cmd;

namespace pika_raft {

// Raft log entry data structure
struct RaftLogEntry {
  std::string cmd_name;
  std::vector<std::string> args;
  std::string db_name;
  int64_t timestamp;
  
  RaftLogEntry() : timestamp(0) {}
  
  // Serialize to string
  std::string SerializeAsString() const;
  
  // Deserialize from string
  bool ParseFromString(const std::string& data);
};

// Write done closure for asynchronous Raft callback
class WriteDoneClosure : public braft::Closure {
 public:
  WriteDoneClosure(std::shared_ptr<Cmd> cmd, 
                   std::shared_ptr<net::RedisConn> conn);
  ~WriteDoneClosure() override = default;
  
  void Run() override;
  
  void SetBinlogData(const std::string& data) { binlog_data_ = data; }
  const std::string& GetBinlogData() const { return binlog_data_; }
  
  // Set promise for synchronous Raft apply
  void SetPromise(std::shared_ptr<std::promise<rocksdb::Status>> p) {
    promise_ = p;
  }

 private:
  std::shared_ptr<Cmd> cmd_;
  std::shared_ptr<net::RedisConn> conn_;
  std::string binlog_data_;
  std::shared_ptr<std::promise<rocksdb::Status>> promise_;
};

// Pika state machine implementation
class PikaStateMachine : public braft::StateMachine {
 public:
  PikaStateMachine();
  ~PikaStateMachine() override = default;

  // Apply committed log entry
  void on_apply(braft::Iterator& iter) override;

  // Save snapshot
  void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;

  // Load snapshot
  int on_snapshot_load(braft::SnapshotReader* reader) override;

  // Leadership changed callback
  void on_leader_start(int64_t term) override;
  void on_leader_stop(const butil::Status& status) override;

  // Error callback
  void on_error(const ::braft::Error& e) override;

  // Configuration changed callback
  void on_configuration_committed(const ::braft::Configuration& conf) override;

  void on_start_following(const ::braft::LeaderChangeContext& ctx) override;
  void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;

 private:
  std::atomic<int64_t> applied_index_;
  std::atomic<int64_t> leader_term_;
};

// Raft node wrapper
class PikaRaftNode {
 public:
  PikaRaftNode(const std::string& group_id, const braft::PeerId& peer_id);
  ~PikaRaftNode();

  // Initialize the Raft node
  pstd::Status Init(const std::vector<braft::PeerId>& peers);

  // Start the Raft node
  pstd::Status Start();

  // Shutdown the Raft node
  void Shutdown();

  // Check if this node is leader
  bool IsLeader() const;

  // Get leader peer ID
  braft::PeerId GetLeaderId();

  // Add peer to the cluster
  pstd::Status AddPeer(const braft::PeerId& peer);

  // Remove peer from the cluster
  pstd::Status RemovePeer(const braft::PeerId& peer);

  // Apply a command to Raft
  pstd::Status Apply(const RaftLogEntry& entry);

  // Get cluster status information
  void GetStatus(std::string* status_str);

  braft::Node* GetRaftNode() { return node_.get(); }

 private:
  std::string group_id_;
  braft::PeerId peer_id_;
  std::unique_ptr<brpc::Server> server_;  // brpc server for Raft RPC
  std::unique_ptr<braft::Node> node_;
  std::unique_ptr<PikaStateMachine> state_machine_;
  
  // Raft data paths
  std::string raft_data_dir_;
  std::string raft_log_uri_;
  std::string raft_meta_uri_;
  std::string raft_snapshot_uri_;
};

// Raft cluster manager
class RaftManager {
 public:
  RaftManager();
  ~RaftManager();

  // Initialize the Raft manager
  pstd::Status Init();

  // Start the Raft manager
  pstd::Status Start();

  // Shutdown the Raft manager
  void Shutdown();

  // Initialize a new Raft cluster
  pstd::Status InitCluster(const std::string& db_name, const std::vector<std::string>& peers);

  // Join an existing Raft cluster
  pstd::Status JoinCluster(const std::string& db_name, const std::string& leader_addr);

  // Add a node to the cluster
  pstd::Status AddNode(const std::string& db_name, const std::string& peer_addr);

  // Remove a node from the cluster
  pstd::Status RemoveNode(const std::string& db_name, const std::string& peer_addr);

  // Get cluster information
  pstd::Status GetClusterInfo(const std::string& db_name, std::string* info);

  // Check if Raft is enabled for a specific DB
  bool IsRaftEnabled(const std::string& db_name) const;

  // Apply a command through Raft
  pstd::Status ApplyCommand(const std::string& db_name, const RaftLogEntry& entry);

  // Apply binlog to Raft (called by storage callback)
  pstd::Status ApplyBinlog(const std::string& db_name, 
                          const std::string& binlog_data,
                          WriteDoneClosure* done);
  
  // Submit command to Raft with promise (for synchronous waiting)
  pstd::Status SubmitCommandWithPromise(const std::string& db_name,
                                        const std::string& log_data,
                                        std::promise<rocksdb::Status>&& promise);
  
  // Apply command from Redis protocol (called in on_apply)
  rocksdb::Status ApplyCommandFromRedisProtocol(const std::string& redis_proto_data,
                                                  const std::string& db_name);

  // Get Raft node for a specific DB
  std::shared_ptr<PikaRaftNode> GetRaftNode(const std::string& db_name);

  // Set storage reference for applying binlog
  void SetStorage(storage::Storage* storage) { storage_ = storage; }

  // Set configuration
  void SetElectionTimeoutMs(int timeout_ms) { election_timeout_ms_ = timeout_ms; }
  void SetSnapshotIntervalS(int interval_s) { snapshot_interval_s_ = interval_s; }

  bool IsInitialized() const { return initialized_.load(); }
  
  // Apply binlog entry to storage (public for PikaStateMachine to call)
  void ApplyBinlogEntry(const std::string& binlog_data);

 private:
  std::atomic<bool> initialized_;
  std::atomic<bool> running_;
  
  // Configuration
  int election_timeout_ms_;
  int snapshot_interval_s_;
  std::string group_id_;
  
  // Raft nodes for each database
  mutable std::shared_mutex nodes_mutex_;
  std::unordered_map<std::string, std::shared_ptr<PikaRaftNode>> raft_nodes_;
  
  // Batch managers for each database (for request batching optimization)
  std::unordered_map<std::string, std::unique_ptr<BatchManager>> batch_managers_;
  
  // Storage reference for applying binlog
  storage::Storage* storage_ = nullptr;
  
  // Helper methods
  pstd::Status CreateRaftNode(const std::string& db_name, const std::vector<braft::PeerId>& peers);
  braft::PeerId ParsePeerId(const std::string& peer_str);
};

}  // namespace pika_raft

#endif  // PRAFT_PRAFT_H_

