/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//
//  praft.cc

#include "praft.h"

#include <cassert>
#include <memory>
#include <string>

#include "client.h"
#include "config.h"
#include "event_loop.h"
#include "log.h"
#include "pikiwidb.h"
#include "pstd_string.h"
#include "replication.h"

#define ERROR_LOG_AND_STATUS(msg) \
  ({                              \
    ERROR(msg);                   \
    butil::Status(EINVAL, msg);   \
  })

namespace pikiwidb {

bool ClusterCmdContext::Set(ClusterCmdType cluster_cmd_type, PClient* client, const std::string& peer_ip, 
      int port, std::string node_id) {
  std::unique_lock<std::mutex> lck(mtx_);
  if (client_ != nullptr) {
    return false;
  }
  assert(client);
  cluster_cmd_type_ = cluster_cmd_type;
  client_ = client;
  peer_ip_ = peer_ip;
  port_ = port;
  node_id_ = node_id;
  return true;
}

void ClusterCmdContext::Clear() {
  std::unique_lock<std::mutex> lck(mtx_);
  cluster_cmd_type_ = ClusterCmdType::NONE;
  client_ = nullptr;
  peer_ip_.clear();
  port_ = 0;
  node_id_.clear();
}

bool ClusterCmdContext::IsEmpty() {
  std::unique_lock<std::mutex> lck(mtx_);
  return client_ == nullptr;
}

void ClusterCmdContext::ConnectTargetNode() {
  auto ip = PREPL.GetMasterAddr().GetIP();
  auto port = PREPL.GetMasterAddr().GetPort();
  if (ip == peer_ip_ && port == port_ && PREPL.GetMasterState() == kPReplStateConnected) {
    return;
  }

  // reconnect
  auto fail_cb = [&](EventLoop*, const char* peer_ip, int port) {
    PRAFT.OnClusterCmdConnectionFailed(EventLoop::Self(), peer_ip, port);
  };
  PREPL.SetFailCallback(fail_cb);
  PREPL.SetMasterState(kPReplStateNone);
  PREPL.SetMasterAddr(peer_ip_.c_str(), port_);
}

PRaft& PRaft::Instance() {
  static PRaft store;
  return store;
}

butil::Status PRaft::Init(std::string& group_id, bool initial_conf_is_null) {
  if (node_ && server_) {
    return {0, "OK"};
  }

  server_ = std::make_unique<brpc::Server>();
  DummyServiceImpl service(&PRAFT);
  auto port = g_config.port + pikiwidb::g_config.raft_port_offset;
  // Add your service into RPC server
  if (server_->AddService(&service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    server_.reset();
    return ERROR_LOG_AND_STATUS("Failed to add service");
  }
  // raft can share the same RPC server. Notice the second parameter, because
  // adding services into a running server is not allowed and the listen
  // address of this server is impossible to get before the server starts. You
  // have to specify the address of the server.
  if (braft::add_service(server_.get(), port) != 0) {
    server_.reset();
    return ERROR_LOG_AND_STATUS("Failed to add raft service");
  }

  // It's recommended to start the server before Counter is started to avoid
  // the case that it becomes the leader while the service is unreacheable by
  // clients.
  // Notice the default options of server is used here. Check out details from
  // the doc of brpc if you would like change some options;
  if (server_->Start(port, nullptr) != 0) {
    server_.reset();
    return ERROR_LOG_AND_STATUS("Failed to start server");
  }

  // It's ok to start PRaft;
  assert(group_id.size() == RAFT_GROUPID_LEN);
  this->group_id_ = group_id;

  // FIXME: g_config.ip is default to 127.0.0.0, which may not work in cluster.
  raw_addr_ = g_config.ip + ":" + std::to_string(port);
  butil::ip_t ip;
  auto ret = butil::str2ip(g_config.ip.c_str(), &ip);
  if (ret != 0) {
    server_.reset();
    return ERROR_LOG_AND_STATUS("Failed to convert str_ip to butil::ip_t");
  }
  butil::EndPoint addr(ip, port);

  // Default init in one node.
  /*
  initial_conf takes effect only when the replication group is started from an empty node.
  The Configuration is restored from the snapshot and log files when the data in the replication group is not empty.
  initial_conf is used only to create replication groups.
  The first node adds itself to initial_conf and then calls add_peer to add other nodes.
  Set initial_conf to empty for other nodes.
  You can also start empty nodes simultaneously by setting the same inital_conf(ip:port of multiple nodes) for multiple
  nodes.
  */
  std::string initial_conf;
  if (!initial_conf_is_null) {
    initial_conf = raw_addr_ + ":0,";
  }
  if (node_options_.initial_conf.parse_from(initial_conf) != 0) {
    server_.reset();
    return ERROR_LOG_AND_STATUS("Failed to parse configuration");
  }

  // node_options_.election_timeout_ms = FLAGS_election_timeout_ms;
  node_options_.fsm = this;
  node_options_.node_owns_fsm = false;
  // node_options_.snapshot_interval_s = FLAGS_snapshot_interval;
  std::string prefix = "local://" + g_config.dbpath + "_praft";
  node_options_.log_uri = prefix + "/log";
  node_options_.raft_meta_uri = prefix + "/raft_meta";
  node_options_.snapshot_uri = prefix + "/snapshot";
  // node_options_.disable_cli = FLAGS_disable_cli;
  node_ = std::make_unique<braft::Node>(group_id, braft::PeerId(addr));
  if (node_->init(node_options_) != 0) {
    server_.reset();
    node_.reset();
    return ERROR_LOG_AND_STATUS("Failed to init raft node");
  }

  return {0, "OK"};
}

bool PRaft::IsLeader() const {
  if (!node_) {
    ERROR("Node is not initialized");
    return false;
  }
  return node_->is_leader();
}

std::string PRaft::GetLeaderId() const {
  if (!node_) {
    ERROR("Node is not initialized");
    return "Failed to get leader id";
  }
  return node_->leader_id().to_string();
}

std::string PRaft::GetNodeId() const {
  if (!node_) {
    ERROR("Node is not initialized");
    return "Failed to get node id";
  }
  return node_->node_id().to_string();
}

std::string PRaft::GetGroupId() const {
  if (!node_) {
    ERROR("Node is not initialized");
    return "Failed to get cluster id";
  }
  return group_id_;
}

braft::NodeStatus PRaft::GetNodeStatus() const {
  braft::NodeStatus node_status;
  if (!node_) {
    ERROR("Node is not initialized");
  } else {
    node_->get_status(&node_status);
  }

  return node_status;
}

butil::Status PRaft::GetListPeers(std::vector<braft::PeerId>* peers) {
  if (!node_) {
    ERROR_LOG_AND_STATUS("Node is not initialized");
  }
  return node_->list_peers(peers);
}

void PRaft::SendNodeRequest(PClient* client) {
  assert(client);

  auto cluster_cmd_type = cluster_cmd_ctx_.GetClusterCmdType();
  switch (cluster_cmd_type) {
    case ClusterCmdContext::ClusterCmdType::JOIN:
      SendNodeInfoRequest(client, "DATA");
      break;
    case ClusterCmdContext::ClusterCmdType::REMOVE:
      SendNodeRemoveRequest(client);
      break;
    default:
      break;
  }
}

// Gets the cluster id, which is used to initialize node
void PRaft::SendNodeInfoRequest(PClient* client, const std::string info_type) {
  assert(client);

  UnboundedBuffer req;
  const std::string cmd_str = "INFO " + info_type;
  req.PushData(cmd_str.c_str(), cmd_str.size());
  req.PushData("\r\n", 2);
  client->SendPacket(req);
  client->Clear();
}

void PRaft::SendNodeAddRequest(PClient* client) {
  assert(client);

  // Node id in braft are ip:port, the node id param in RAFT.NODE ADD cmd will be ignored.
  int unused_node_id = 0;
  auto port = g_config.port + pikiwidb::g_config.raft_port_offset;
  auto raw_addr = g_config.ip + ":" + std::to_string(port);
  UnboundedBuffer req;
  req.PushData("RAFT.NODE ADD ", 14);
  req.PushData(std::to_string(unused_node_id).c_str(), std::to_string(unused_node_id).size());
  req.PushData(" ", 1);
  req.PushData(raw_addr.data(), raw_addr.size());
  req.PushData("\r\n", 2);
  client->SendPacket(req);
  client->Clear();
}

void PRaft::SendNodeRemoveRequest(PClient* client) {
  assert(client);

  UnboundedBuffer req;
  req.PushData("RAFT.NODE REMOVE ", 17);
  req.PushData(cluster_cmd_ctx_.GetNodeID().c_str(), cluster_cmd_ctx_.GetNodeID().size());
  req.PushData("\r\n", 2);
  client->SendPacket(req);
  client->Clear();
}

int PRaft::ProcessClusterCmdResponse(PClient* client, const char* start, int len) {
  auto cluster_cmd_type = cluster_cmd_ctx_.GetClusterCmdType();
  int ret = 0;
  switch (cluster_cmd_type) {
    case ClusterCmdContext::ClusterCmdType::JOIN:
      ret = PRAFT.ProcessClusterJoinCmdResponse(client, start, len);
      break;
    case ClusterCmdContext::ClusterCmdType::REMOVE:
      ret = PRAFT.ProcessClusterRemoveCmdResponse(client, start, len);
      break;
    default:
      break;
  }

  return ret;
}

int PRaft::ProcessClusterJoinCmdResponse(PClient* client, const char* start, int len) {
  assert(start);
  auto join_client = cluster_cmd_ctx_.GetClient();
  if (!join_client) {
    WARN("No client when processing cluster join cmd response.");
    return 0;
  }

  std::string reply(start, len);
  if (reply.find("+OK") != std::string::npos) {
    INFO("Joined Raft cluster, node id: {}, group_id: {}", PRAFT.GetNodeId(), PRAFT.group_id_);
    join_client->SetRes(CmdRes::kOK);
    join_client->SendPacket(join_client->Message());
    join_client->Clear();
    // If the join fails, clear clusterContext and set it again by using the join command
    cluster_cmd_ctx_.Clear(); 
  } else if (reply.find("databases_num") != std::string::npos) {
    int databases_num = 0;
    int rocksdb_num = 0;
    std::string rockdb_version;
    std::string line;
    std::istringstream iss(reply);

    while (std::getline(iss, line)) {
      std::string::size_type pos = line.find(':');
      if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "databases_num") {
          databases_num = std::stoi(value);
        } else if (key == "rocksdb_num") {
          rocksdb_num = std::stoi(value);
        } else if (key == "rockdb_version") {
          rockdb_version = pstd::StringTrimRight(value, "\r");
        }
      }
    }

    int current_databases_num = pikiwidb::g_config.databases;
    int current_rocksdb_num = pikiwidb::g_config.db_instance_num;
    std::string current_rocksdb_version = ROCKSDB_NAMESPACE::GetRocksVersionAsString();
    if (current_databases_num != databases_num || current_rocksdb_num != rocksdb_num || current_rocksdb_version != rockdb_version) {
      join_client->SetRes(CmdRes::kErrOther, "Config of databases_num, rocksdb_num or rocksdb_version mismatch");
      join_client->SendPacket(join_client->Message());
      join_client->Clear();
      // If the join fails, clear clusterContext and set it again by using the join command
      cluster_cmd_ctx_.Clear(); 
    } else {
      SendNodeInfoRequest(client, "RAFT");
    }
  } else if (reply.find("-ERR wrong leader") != std::string::npos) {
    // Resolve the ip address of the leader
    pstd::StringTrimLeft(reply, "-ERR wrong leader");
    pstd::StringTrim(reply);
    braft::PeerId peerId;
    peerId.parse(reply);
    auto peer_ip = std::string(butil::ip2str(peerId.addr.ip).c_str());
    auto port = peerId.addr.port;

    // Reset the target of the connection
    cluster_cmd_ctx_.Clear(); 
    auto ret = PRAFT.GetClusterCmdCtx().Set(ClusterCmdContext::ClusterCmdType::JOIN, join_client, peer_ip, port);
    if (!ret) {  // other clients have joined
      join_client->SetRes(CmdRes::kErrOther, "Other clients have joined");
      join_client->SendPacket(join_client->Message());
      join_client->Clear();
      return len;
    }
    PRAFT.GetClusterCmdCtx().ConnectTargetNode();

    // Not reply any message here, we will reply after the connection is established.
    join_client->Clear();
  } else if (reply.find("raft_group_id") != std::string::npos) {
    std::string prefix = "raft_group_id:";
    std::string::size_type prefix_length = prefix.length();
    std::string::size_type group_id_start = reply.find(prefix);
    group_id_start += prefix_length;  // 定位到raft_group_id的起始位置
    std::string::size_type group_id_end = reply.find("\r\n", group_id_start);
    if (group_id_end != std::string::npos) {
      std::string raft_group_id = reply.substr(group_id_start, group_id_end - group_id_start);
      // initialize the slave node
      auto s = PRAFT.Init(raft_group_id, true);
      if (!s.ok()) {
        join_client->SetRes(CmdRes::kErrOther, s.error_str());
        join_client->SendPacket(join_client->Message());
        join_client->Clear();
        // If the join fails, clear clusterContext and set it again by using the join command
        cluster_cmd_ctx_.Clear(); 
        return len;
      }

      PRAFT.SendNodeAddRequest(client);
    } else {
      ERROR("Joined Raft cluster fail, because of invalid raft_group_id");
      join_client->SetRes(CmdRes::kErrOther, "Invalid raft_group_id");
      join_client->SendPacket(join_client->Message());
      join_client->Clear();
      // If the join fails, clear clusterContext and set it again by using the join command
      cluster_cmd_ctx_.Clear(); 
    }
  } else {
    ERROR("Joined Raft cluster fail, str: {}", start);
    join_client->SetRes(CmdRes::kErrOther, std::string(start, len));
    join_client->SendPacket(join_client->Message());
    join_client->Clear();
    // If the join fails, clear clusterContext and set it again by using the join command
    cluster_cmd_ctx_.Clear(); 
  }

  return len;
}

int PRaft::ProcessClusterRemoveCmdResponse(PClient* client, const char* start, int len) {
  assert(start);
  auto remove_client = cluster_cmd_ctx_.GetClient();
  if (!remove_client) {
    WARN("No client when processing cluster remove cmd response.");
    return 0;
  }

  std::string reply(start, len);
  if (reply.find("+OK") != std::string::npos) {
    INFO("Removed Raft cluster, node id: {}, group_id: {}", PRAFT.GetNodeId(), PRAFT.group_id_);
    remove_client->SetRes(CmdRes::kOK);
    remove_client->SendPacket(remove_client->Message());
    remove_client->Clear();
  } else {
    ERROR("Removed Raft cluster fail, str: {}", start);
    remove_client->SetRes(CmdRes::kErrOther, std::string(start, len));
    remove_client->SendPacket(remove_client->Message());
    remove_client->Clear();
    
  }

  // If the remove fails, clear clusterContext and set it again by using the join command
  cluster_cmd_ctx_.Clear(); 

  return len;
}

butil::Status PRaft::AddPeer(const std::string& peer) {
  if (!node_) {
    ERROR_LOG_AND_STATUS("Node is not initialized");
  }

  braft::SynchronizedClosure done;
  node_->add_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    WARN("Failed to add peer {} to node {}, status: {}", peer, node_->node_id().to_string(), done.status().error_str());
    return done.status();
  }

  return {0, "OK"};
}

butil::Status PRaft::RemovePeer(const std::string& peer) {
  if (!node_) {
    return ERROR_LOG_AND_STATUS("Node is not initialized");
  }

  braft::SynchronizedClosure done;
  node_->remove_peer(peer, &done);
  done.wait();

  if (!done.status().ok()) {
    WARN("Failed to remove peer {} from node {}, status: {}", peer, node_->node_id().to_string(),
         done.status().error_str());
    return done.status();
  }

  return {0, "OK"};
}

void PRaft::OnClusterCmdConnectionFailed([[maybe_unused]] EventLoop* loop, const char* peer_ip, int port) {
  auto cli = cluster_cmd_ctx_.GetClient();
  if (cli) {
    cli->SetRes(CmdRes::kErrOther, "ERR failed to connect to cluster for join or remove, please check logs " +
                                       std::string(peer_ip) + ":" + std::to_string(port));
    cli->SendPacket(cli->Message());
    cli->Clear();
  }
  cluster_cmd_ctx_.Clear();

  // if (PRAFT.IsInitialized()) {
  //   PRAFT.ShutDown();
  //   PRAFT.Join();
  // }
}

// Shut this node and server down.
void PRaft::ShutDown() {
  if (node_) {
    node_->shutdown(nullptr);
  }

  if (server_) {
    server_->Stop(0);
  }
}

// Blocking this thread until the node is eventually down.
void PRaft::Join() {
  if (node_) {
    node_->join();
  }

  if (server_) {
    server_->Join();
  }
}

void PRaft::Apply(braft::Task& task) {
  if (node_) {
    node_->apply(task);
  }
}

// @braft::StateMachine
void PRaft::on_apply(braft::Iterator& iter) {
  // A batch of tasks are committed, which must be processed through
  // |iter|
  for (; iter.valid(); iter.next()) {
  }
}

void PRaft::on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) {}

int PRaft::on_snapshot_load(braft::SnapshotReader* reader) { return 0; }

void PRaft::on_leader_start(int64_t term) {
  WARN("Node {} start to be leader, term={}", node_->node_id().to_string(), term);
}

void PRaft::on_leader_stop(const butil::Status& status) {}

void PRaft::on_shutdown() {}
void PRaft::on_error(const ::braft::Error& e) {}
void PRaft::on_configuration_committed(const ::braft::Configuration& conf) {}
void PRaft::on_stop_following(const ::braft::LeaderChangeContext& ctx) {}
void PRaft::on_start_following(const ::braft::LeaderChangeContext& ctx) {}

}  // namespace pikiwidb
