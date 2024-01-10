/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_raft.h"
#include <cstdint>
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

    // (KKorpse)TODO: Redirect to leader if not leader.
    auto s = PRAFT.add_peer(client->argv_[3]);
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
    auto s = PRAFT.remove_peer(client->argv_[2]);
    if (s.ok()) {
      client->SetRes(CmdRes::kOK);
    } else {
      client->SetRes(CmdRes::kErrOther);
    }

  } else {
    client->SetRes(CmdRes::kErrOther, "ERR RAFT.NODE supports ADD / REMOVE only");
  }

  // (KKorpse)TODO: Support all replys types:
  //   -NOCLUSTER ||
  //   -LOADING ||
  //   -CLUSTERDOWN ||
  //   -MOVED <slot> <addr>:<port> ||
  //   *2
  //   :<new node id>
  //   :<dbid>
}

}  // namespace pikiwidb