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

class PikaServer;

// Forward declarations
namespace storage {
class Storage;
}

namespace pikiwidb {
class Binlog;
}

namespace pika_raft {

// Raft log entry data structure
// Write done closure for asynchronous Raft callback
class WriteDoneClosure : public braft::Closure {
 public:
  WriteDoneClosure() = default;
  ~WriteDoneClosure() override = default;
  
  void Run() override;
  
  // Set promise for synchronous Raft apply
  void SetPromise(std::shared_ptr<std::promise<rocksdb::Status>> p) {
    promise_ = p;
  }

 private:
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

  // Append binlog
  void AppendLog(const std::string& db_name, 
                 const ::pikiwidb::Binlog& log, 
                 std::promise<rocksdb::Status>&& promise);
  
  // Get Raft node for a specific DB
  std::shared_ptr<PikaRaftNode> GetRaftNode(const std::string& db_name);
  
  // Apply binlog entry to storage (public for PikaStateMachine to call)
  rocksdb::Status ApplyBinlogEntry(const std::string& binlog_data);

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
  
  // Helper methods
  pstd::Status CreateRaftNode(const std::string& db_name, const std::vector<braft::PeerId>& peers);
  braft::PeerId ParsePeerId(const std::string& peer_str);
};

}  // namespace pika_raft

#endif  // PRAFT_PRAFT_H_

