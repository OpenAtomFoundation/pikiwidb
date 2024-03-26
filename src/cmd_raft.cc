/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <cstdint>
#include <optional>
#include <string>

#include "client.h"
#include "cmd_raft.h"
#include "event_loop.h"
#include "log.h"
#include "pikiwidb.h"
#include "praft.h"
#include "pstd_string.h"

namespace pikiwidb {

RaftNodeCmd::RaftNodeCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsRaft, kAclCategoryRaft) {}

bool RaftNodeCmd::DoInitial(PClient* client) { return true; }

void RaftNodeCmd::DoCmd(PClient* client) {
  // Check whether it is a leader. If it is not a leader, return the leader information
  if (!PRAFT.IsLeader()) {
    return client->SetRes(CmdRes::kWrongLeader, PRAFT.GetLeaderId());
  }

  auto cmd = client->argv_[1];
  pstd::StringToUpper(cmd);
  if (!strcasecmp(cmd.c_str(), "ADD")) {
    DoCmdAdd(client);
  } else if (!strcasecmp(cmd.c_str(), "REMOVE")) {
    DoCmdRemove(client);
  } else if (!strcasecmp(cmd.c_str(), "DSS")) {
    DoCmdSnapshot(client);
  } else {
    client->SetRes(CmdRes::kErrOther, "RAFT.NODE supports ADD / REMOVE only");
  }
}

void RaftNodeCmd::DoCmdAdd(PClient* client) {
  if (client->argv_.size() != 4) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  // RedisRaft has nodeid, but in Braft, NodeId is IP:Port.
  // So we do not need to parse and use nodeid like redis;
  auto s = PRAFT.AddPeer(client->argv_[3]);
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, fmt::format("Failed to add peer: {}", s.error_str()));
  }
}

void RaftNodeCmd::DoCmdRemove(PClient* client) {
  if (client->argv_.size() != 3) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  // (KKorpse)TODO: Redirect to leader if not leader.
  auto s = PRAFT.RemovePeer(client->argv_[2]);
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, fmt::format("Failed to remove peer: {}", s.error_str()));
  }
}

void RaftNodeCmd::DoCmdSnapshot(PClient* client) {
  auto s = PRAFT.DoSnapshot();
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  }
}

RaftClusterCmd::RaftClusterCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsRaft, kAclCategoryRaft) {}

bool RaftClusterCmd::DoInitial(PClient* client) { return true; }

void RaftClusterCmd::DoCmd(PClient* client) {
  // parse arguments
  if (client->argv_.size() < 2) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }
  auto cmd = client->argv_[1];

  if (PRAFT.IsInitialized()) {
    return client->SetRes(CmdRes::kErrOther, "Already cluster member");
  }

  pstd::StringToUpper(cmd);
  if (cmd == kInitCmd) {
    DoCmdInit(client);
  } else if (cmd == kJoinCmd) {
    DoCmdJoin(client);
  } else {
    client->SetRes(CmdRes::kErrOther, "RAFT.CLUSTER supports INIT/JOIN only");
  }
}

void RaftClusterCmd::DoCmdInit(PClient* client) {
  if (client->argv_.size() != 2 && client->argv_.size() != 3) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  std::string cluster_id;
  if (client->argv_.size() == 3) {
    cluster_id = client->argv_[2];
    if (cluster_id.size() != RAFT_DBID_LEN) {
      return client->SetRes(CmdRes::kInvalidParameter,
                            "Cluster id must be " + std::to_string(RAFT_DBID_LEN) + " characters");
    }
  } else {
    cluster_id = pstd::RandomHexChars(RAFT_DBID_LEN);
  }
  auto s = PRAFT.Init(cluster_id, false);
  if (!s.ok()) {
    return client->SetRes(CmdRes::kErrOther, fmt::format("Failed to init node: ", s.error_str()));
  }
  client->SetRes(CmdRes::kOK);
}

static inline std::optional<std::pair<std::string, int32_t>> GetIpAndPortFromEndPoint(const std::string& endpoint) {
  auto pos = endpoint.find(':');
  if (pos == std::string::npos) {
    return std::nullopt;
  }

  int32_t ret = 0;
  pstd::String2int(endpoint.substr(pos + 1), &ret);
  return {{endpoint.substr(0, pos), ret}};
}

void RaftClusterCmd::DoCmdJoin(PClient* client) {
  if (client->argv_.size() < 3) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  // (KKorpse)TODO: Support multiple nodes join at the same time.
  if (client->argv_.size() > 3) {
    return client->SetRes(CmdRes::kInvalidParameter, "Too many arguments");
  }

  auto addr = client->argv_[2];
  if (braft::PeerId(addr).is_empty()) {
    return client->SetRes(CmdRes::kErrOther, fmt::format("Invalid ip::port: {}", addr));
  }

  auto on_new_conn = [](TcpConnection* obj) {
    if (g_pikiwidb) {
      g_pikiwidb->OnNewConnection(obj);
    }
  };
  auto on_fail = [&](EventLoop* loop, const char* peer_ip, int port) {
    PRAFT.OnJoinCmdConnectionFailed(loop, peer_ip, port);
  };

  auto loop = EventLoop::Self();
  auto ip_port = GetIpAndPortFromEndPoint(addr);
  if (!ip_port.has_value()) {
    return client->SetRes(CmdRes::kErrOther, fmt::format("Invalid ip::port: {}", addr));
  }
  auto& [peer_ip, port] = *ip_port;
  // FIXME: The client here is not smart pointer, may cause undefined behavior.
  // should use shared_ptr in DoCmd() rather than raw pointer.
  auto ret = PRAFT.GetJoinCtx().Set(client, peer_ip, port);
  if (!ret) {  // other clients have joined
    return client->SetRes(CmdRes::kErrOther, "Other clients have joined");
  }
  loop->Connect(peer_ip.c_str(), port, on_new_conn, on_fail);
  INFO("Sent join request to leader successfully");
  // Not reply any message here, we will reply after the connection is established.
  client->Clear();
}

}  // namespace pikiwidb