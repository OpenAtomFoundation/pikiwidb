/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>
#include <string>

#include "braft/configuration.h"
#include "braft/raft.h"
#include "braft/util.h"
#include "brpc/controller.h"
#include "brpc/server.h"
#include "butil/status.h"
#include "praft.pb.h"

#include "client.h"
#include "event_loop.h"
#include "tcp_connection.h"

namespace pikiwidb {

#define RAFT_GROUPID_LEN 32

#define PRAFT PRaft::Instance()

class ClusterCmdContext {
  friend class PRaft;

 public:
  enum ClusterCmdType{
    NONE,
    JOIN,
    REMOVE,
  };

  ClusterCmdContext() = default;
  ~ClusterCmdContext() = default;

  bool Set(ClusterCmdType cluster_cmd_type, PClient* client, const std::string& peer_ip, 
        int port, std::string node_id = "");

  void Clear();

  // @todo the function seems useless
  bool IsEmpty();

  ClusterCmdType GetClusterCmdType() { return cluster_cmd_type_; }
  PClient* GetClient() { return client_; }
  const std::string& GetPeerIp() { return peer_ip_; }
  int GetPort() { return port_; }
  const std::string& GetNodeID() { return node_id_; }

  void ConnectTargetNode();

 private:
  ClusterCmdType cluster_cmd_type_ = ClusterCmdType::NONE;
  std::mutex mtx_;
  PClient* client_ = nullptr;
  std::string peer_ip_;
  int port_ = 0;
  std::string node_id_;
};

class PRaft : public braft::StateMachine {
 public:
  PRaft() : server_(nullptr), node_(nullptr) {}

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
  void Apply(braft::Task& task);

  //===--------------------------------------------------------------------===//
  // Cluster command
  //===--------------------------------------------------------------------===//
  ClusterCmdContext& GetClusterCmdCtx() { return cluster_cmd_ctx_; }
  void SendNodeRequest(PClient* client);
  void SendNodeInfoRequest(PClient* client, const std::string info_type);
  void SendNodeAddRequest(PClient* client);
  void SendNodeRemoveRequest(PClient* client); 

  int ProcessClusterCmdResponse(PClient* client, const char* start, int len);
  int ProcessClusterJoinCmdResponse(PClient* client, const char* start, int len);
  int ProcessClusterRemoveCmdResponse(PClient* client, const char* start, int len);

  void OnClusterCmdConnectionFailed(EventLoop*, const char* peer_ip, int port);

  bool IsLeader() const;
  std::string GetLeaderId() const;
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
  std::unique_ptr<brpc::Server> server_;  // brpc
  std::unique_ptr<braft::Node> node_;
  braft::NodeOptions node_options_;  // options for raft node
  std::string raw_addr_;             // ip:port of this node

  ClusterCmdContext cluster_cmd_ctx_;  // context for cluster join/remove command
  std::string group_id_;         // group id
};

class DummyServiceImpl : public DummyService {
 public:
  explicit DummyServiceImpl(PRaft* praft) : praft_(praft) {}
  void DummyMethod(::google::protobuf::RpcController* controller, const ::pikiwidb::DummyRequest* request,
                   ::pikiwidb::DummyResponse* response, ::google::protobuf::Closure* done) {}

 private:
  PRaft* praft_;
};

}  // namespace pikiwidb
