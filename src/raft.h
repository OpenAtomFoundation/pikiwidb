/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "braft/configuration.h"
#include "braft/raft.h"
#include "braft/util.h"
#include "butil/status.h"
#include "gflags/gflags.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include "client.h"
#include "common.h"

namespace pikiwidb {

#define RAFT_DBID_LEN 32
#define RAFT_PORT_OFFSET 10

#define PRAFT PRaft::Instance()

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
    ;
    port_ = port;
    return true;
  }

  void Clear() {
    std::unique_lock<std::mutex> lck(mtx_);
    client_ = nullptr;
    peer_ip_.clear();
    port_ = 0;
  }

  bool IsEmpty() {
    std::unique_lock<std::mutex> lck(mtx_);
    return client_ == nullptr;
  }

  PClient* GetClient() { return client_; }
  braft::PeerId GetPeerIp() { return peer_ip_; }
  int GetPort() { return port_; }

 private:
  PClient* client_ = nullptr;
  std::string peer_ip_;
  int port_ = 0;
  std::mutex mtx_;
};

class PRaft : public braft::StateMachine {
 public:
  PRaft() : node_(nullptr) {}

  ~PRaft() override = default;

  static PRaft& Instance();

  //===--------------------------------------------------------------------===//
  // Braft API
  //===--------------------------------------------------------------------===//
  butil::Status Init(std::string& cluster_id);
  butil::Status AddPeer(const std::string& peer);
  butil::Status RemovePeer(const std::string& peer);
  butil::Status RaftRecvEntry();

  void ShutDown();
  void Join();
  
  //===--------------------------------------------------------------------===//
  // ClusterJoin command
  //===--------------------------------------------------------------------===//
  JoinCmdContext& GetJoinCtx() { return join_ctx_; }
  void SendNodeAddRequest(PClient *client);
  int ProcessClusterJoinCmdResponse(const char* start, int len);
  void OnJoinCmdConnectionFailed(EventLoop*, const char* peer_ip, int port);

  bool IsLeader() const;
  bool IsInitialized() const { return node_ != nullptr; }

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
  std::unique_ptr<braft::Node> node_;
  braft::NodeOptions node_options_;  // options for raft node
  std::string raw_addr_;             // ip:port of this node

  JoinCmdContext join_ctx_;  // context for cluster join command
  std::string _dbid;         // dbid of cluster,
};

}  // namespace pikiwidb