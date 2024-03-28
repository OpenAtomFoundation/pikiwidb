/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <future>

#include "braft/raft.h"
#include "rocksdb/status.h"

namespace pikiwidb {

#define RAFT_DBID_LEN 32

#define PRAFT PRaft::Instance()

class PClient;
class EventLoop;
class Binlog;

class JoinCmdContext {
  friend class PRaft;

 public:
  JoinCmdContext() = default;
  ~JoinCmdContext() = default;

  bool Set(PClient* client, const std::string& peer_ip, int port) {
    std::unique_lock<std::mutex> lck(mtx_);
    if (client_ != nullptr) {
      return false;
    }
    assert(client);
    client_ = client;
    peer_ip_ = peer_ip;
    port_ = port;
    return true;
  }

  void Clear() {
    std::unique_lock<std::mutex> lck(mtx_);
    client_ = nullptr;
    peer_ip_.clear();
    port_ = 0;
  }

  // @todo the function seems useless
  bool IsEmpty() {
    std::unique_lock<std::mutex> lck(mtx_);
    return client_ == nullptr;
  }

  PClient* GetClient() { return client_; }
  const std::string& GetPeerIp() { return peer_ip_; }
  int GetPort() { return port_; }

 private:
  std::mutex mtx_;
  PClient* client_ = nullptr;
  std::string peer_ip_;
  int port_ = 0;
};

class PRaftWriteDoneClosure : public braft::Closure {
 public:
  explicit PRaftWriteDoneClosure(std::promise<rocksdb::Status>&& promise) : promise_(std::move(promise)) {}

  void Run() override {
    promise_.set_value(result_);
    delete this;
  }
  void SetStatus(rocksdb::Status status) { result_ = std::move(status); }

 private:
  std::promise<rocksdb::Status> promise_;
  rocksdb::Status result_{rocksdb::Status::Aborted("Unknown error")};
};

class PRaft : public braft::StateMachine {
 public:
  PRaft() = default;
  ~PRaft() override = default;

  static PRaft& Instance();

  //===--------------------------------------------------------------------===//
  // Braft API
  //===--------------------------------------------------------------------===//
  butil::Status Init(std::string& group_id, bool initial_conf_is_null);
  butil::Status AddPeer(const std::string& peer);
  butil::Status RemovePeer(const std::string& peer);
  butil::Status RaftRecvEntry();

  void ShutDown();
  void Join();
  void AppendLog(const Binlog& log, std::promise<rocksdb::Status>&& promise);

  //===--------------------------------------------------------------------===//
  // ClusterJoin command
  //===--------------------------------------------------------------------===//
  JoinCmdContext& GetJoinCtx() { return join_ctx_; }
  void SendNodeInfoRequest(PClient* client);
  void SendNodeAddRequest(PClient* client);
  std::tuple<int, bool> ProcessClusterJoinCmdResponse(PClient* client, const char* start, int len);
  void OnJoinCmdConnectionFailed(EventLoop*, const char* peer_ip, int port);

  bool IsLeader() const;
  std::string GetLeaderId() const;
  std::string GetLeaderAddress() const;
  std::string GetNodeId() const;
  std::string GetGroupId() const;
  braft::NodeStatus GetNodeStatus() const;
  butil::Status GetListPeers(std::vector<braft::PeerId>* peers);

  bool IsInitialized() const { return node_ != nullptr && server_ != nullptr; }

 private:
  void on_apply(braft::Iterator& iter) override;
  void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
  int on_snapshot_load(braft::SnapshotReader* reader) override;

  void on_leader_start(int64_t term) override;
  void on_leader_stop(const butil::Status& status) override;

  void on_shutdown() override;
  void on_error(const ::braft::Error& e) override;
  void on_configuration_committed(const ::braft::Configuration& conf) override;
  void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;
  void on_start_following(const ::braft::LeaderChangeContext& ctx) override;

 private:
  std::unique_ptr<brpc::Server> server_{nullptr};  // brpc
  std::unique_ptr<braft::Node> node_{nullptr};
  braft::NodeOptions node_options_;  // options for raft node
  std::string raw_addr_;             // ip:port of this node

  JoinCmdContext join_ctx_;  // context for cluster join command
  std::string dbid_;         // dbid of group,
};

}  // namespace pikiwidb
