/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_raft.h"
#include <braft/configuration.h>
#include <cassert>
#include <cstdint>
#include <string>
#include "client.h"
#include "event_loop.h"
#include "pikiwidb.h"
#include "pstd_status.h"
#include "pstd_string.h"
#include "raft.h"

#define VALID_NODE_ID(x) ((x) > 0)

namespace pikiwidb {

bool RaftNodeCmd::DoInitial(PClient* client) { return true; }

/* RAFT.NODE ADD [id] [address:port]
 *   Add a new node to the cluster.  The [id] can be an explicit non-zero value,
 *   or zero to let the cluster choose one.
 * Reply:
 *   -NOCLUSTER ||
 *   -LOADING ||
 *   -CLUSTERDOWN ||
 *   -MOVED <slot> <addr>:<port> ||
 *   *2
 *   :<new node id>
 *   :<dbid>
 *
 * RAFT.NODE REMOVE [id]
 *   Remove an existing node from the cluster.
 * Reply:
 *   -NOCLUSTER ||
 *   -LOADING ||
 *   -CLUSTERDOWN ||
 *   -MOVED <slot> <addr>:<port> ||
 *   +OK
 */
void RaftNodeCmd::DoCmd(PClient* client) {
  auto cmd = client->argv_[1];

  if (!strcasecmp(cmd.c_str(), "ADD")) {
    if (client->argv_.size() != 4) {
      return client->SetRes(CmdRes::kWrongNum, client->CmdName());
    }

    // RedisRaft has nodeid, but in Braft, NodeId is IP:Port.
    // So we do not need to parse and use nodeid like redis;

    auto s = PRAFT.AddPeer(client->argv_[3]);
    if (s.ok()) {
      client->SetRes(CmdRes::kOK);
    } else {
      client->SetRes(CmdRes::kErrOther);
    }

  } else if (!strcasecmp(cmd.c_str(), "REMOVE")) {
    if (client->argv_.size() != 3) {
      return client->SetRes(CmdRes::kWrongNum, client->CmdName());
    }

    // (KKorpse)TODO: Redirect to leader if not leader.
    auto s = PRAFT.RemovePeer(client->argv_[2]);
    if (s.ok()) {
      client->SetRes(CmdRes::kOK);
    } else {
      client->SetRes(CmdRes::kErrOther);
    }

  } else {
    client->SetRes(CmdRes::kErrOther, "ERR RAFT.NODE supports ADD / REMOVE only");
  }
}

bool RaftClusterCmd::DoInitial(PClient* client) { return true; }

// The endpoint must be in the league format of ip:port
std::string GetIpFromEndPoint(std::string& endpoint) {
  auto pos = endpoint.find(':');
  assert(pos != std::string::npos);
  if (pos == std::string::npos) {
    return "";
  }
  return endpoint.substr(0, pos);
}

// The endpoint must be in the league format of ip:port
int GetPortFromEndPoint(std::string& endpoint) {
  auto pos = endpoint.find(':');
  assert(pos != std::string::npos);
  if (pos == std::string::npos) {
    return 0;
  }
  int ret = 0;
  pstd::String2int(endpoint.substr(pos + 1), &ret);
  return ret;
}

/* RAFT.CLUSTER INIT <id>
 *   Initializes a new Raft cluster.
 *   <id> is an optional 32 character string, if set, cluster will use it for the id
 * Reply:
 *   +OK [dbid]
 *
 * RAFT.CLUSTER JOIN [addr:port]
 *   Join an existing cluster.
 *   The operation is asynchronous and may take place/retry in the background.
 * Reply:
 *   +OK
 */
void RaftClusterCmd::DoCmd(PClient* client) {
  if (client->argv_.size() < 2) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  if (PRAFT.IsInitialized()) {
    return client->SetRes(CmdRes::kErrOther, "ERR Already cluster member");
  }

  auto cmd = client->argv_[1];
  if (!strcasecmp(cmd.c_str(), "INIT")) {
    if (client->argv_.size() != 2 && client->argv_.size() != 3) {
      return client->SetRes(CmdRes::kWrongNum, client->CmdName());
    }

    std::string cluster_id;
    if (client->argv_.size() == 3) {
      cluster_id = client->argv_[2];
      if (cluster_id.size() != RAFT_DBID_LEN) {
        return client->SetRes(CmdRes::kInvalidParameter,
                              "ERR cluster id must be " + std::to_string(RAFT_DBID_LEN) + " characters");
      }
    } else {
      cluster_id = pstd::RandomHexChars(RAFT_DBID_LEN);
    }

    auto s = PRAFT.Init(cluster_id);
    if (!s.ok()) {
      return client->SetRes(CmdRes::kErrOther, s.error_str());
    }
    client->SetRes(CmdRes::kOK, "OK " + cluster_id);

  } else if (!strcasecmp(cmd.c_str(), "JOIN")) {
    if (client->argv_.size() < 3) {
      return client->SetRes(CmdRes::kWrongNum, client->CmdName());
    }

    // (KKorpse)TODO: Support multiple nodes join at the same time.
    if (client->argv_.size() > 3) {
      return client->SetRes(CmdRes::kInvalidParameter, "ERR too many arguments");
    }

    auto addr = client->argv_[2];
    if (braft::PeerId(addr).is_empty()) {
      return client->SetRes(CmdRes::kInvalidParameter, "ERR invalid ip::port: " + addr);
    }

    auto on_new_conn = [](TcpConnection* obj) {
      if (g_pikiwidb) {
        g_pikiwidb->OnNewConnection(obj);
      }
    };
    auto fail_cb = [&](EventLoop* loop, const char* peer_ip, int port) {
      PRAFT.OnJoinCmdConnectionFailed(loop, peer_ip, port);
    };

    auto loop = EventLoop::Self();
    auto peer_ip = GetIpFromEndPoint(addr);
    auto port = GetPortFromEndPoint(addr);
    // FIXME: The client here is not smart pointer, may cause undefined behavior.
    // should use shared_ptr in DoCmd() rather than raw pointer.
    PRAFT.GetJoinCtx().Set(client, peer_ip, port);
    loop->Connect(peer_ip.c_str(), port, on_new_conn, fail_cb);

    // Not reply any message here, we will reply after the connection is established.
    client->Clear();

  } else {
    client->SetRes(CmdRes::kErrOther, "ERR RAFT.CLUSTER supports INIT / JOIN only");
  }
}
}  // namespace pikiwidb