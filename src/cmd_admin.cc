/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_admin.h"
#include "store.h"
#include "braft/raft.h"
#include "praft.h"

namespace pikiwidb {

CmdConfig::CmdConfig(const std::string& name, int arity) : BaseCmdGroup(name, kCmdFlagsAdmin, kAclCategoryAdmin) {}

bool CmdConfig::HasSubCommand() const { return true; }

CmdConfigGet::CmdConfigGet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryAdmin) {}

bool CmdConfigGet::DoInitial(PClient* client) { return true; }

void CmdConfigGet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

CmdConfigSet::CmdConfigSet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin, kAclCategoryAdmin) {}

bool CmdConfigSet::DoInitial(PClient* client) { return true; }

void CmdConfigSet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

FlushdbCmd::FlushdbCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryAdmin) {}

bool FlushdbCmd::DoInitial(PClient* client) { return true; }

void FlushdbCmd::DoCmd(PClient* client) {
  PSTORE.dirty_ += PSTORE.DBSize();
  PSTORE.ClearCurrentDB();
  Propagate(PSTORE.GetDB(), std::vector<PString>{"flushdb"});
  client->SetRes(CmdRes::kOK);
}

FlushallCmd::FlushallCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryAdmin) {}

bool FlushallCmd::DoInitial(PClient* client) { return true; }

void FlushallCmd::DoCmd(PClient* client) {
  int currentDB = PSTORE.GetDB();
  std::vector<PString> param{"flushall"};
  DEFER {
    PSTORE.SelectDB(currentDB);
    Propagate(-1, param);
    PSTORE.ResetDB();
  };

  for (int dbno = 0; true; ++dbno) {
    if (PSTORE.SelectDB(dbno) == -1) {
      break;
    }
    PSTORE.dirty_ += PSTORE.DBSize();
  }
  client->SetRes(CmdRes::kOK);
}

InfoCmd::InfoCmd(const std::string& name, int16_t arity) 
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsReadonly, kAclCategoryAdmin) {}

bool InfoCmd::DoInitial(PClient* client) { return true; }

/*
* INFO raft
* Querying Node Information.
* Reply:
*   raft_group_id:595100767
    raft_state:up
    raft_role:follower
    raft_is_voting:yes
    raft_leader_id:1733428433
    raft_current_term:1
    raft_num_nodes:2
    raft_num_voting_nodes:2
    raft_node1:id=1733428433,state=connected,voting=yes,addr=localhost,port=5001,last_conn_secs=5,conn_errors=0,conn_oks=1
*/
// @todo The info raft command is only supported for the time being
void InfoCmd::DoCmd(PClient* client) {
  if (client->argv_.size() < 1) {
    return client->SetRes(CmdRes::kWrongNum, client->CmdName());
  }

  auto cmd = client->argv_[1];
  if (!strcasecmp(cmd.c_str(), "RAFT")) {
    if (client->argv_.size() != 2) {
      return client->SetRes(CmdRes::kWrongNum, client->CmdName());
    }

    if (!PRAFT.IsInitialized()) {
      return client->SetRes(CmdRes::kErrOther, "don't already cluster member");
    }

    auto node_status = PRAFT.GetNodeStatus();
    if (node_status.state == braft::State::STATE_END) {
      return client->SetRes(CmdRes::kErrOther, "Node is not initialized");
    }

    std::string message("");
    message += "raft_group_id:" + PRAFT.GetGroupId() + "\r\n";
    message += "raft_state:" + std::to_string(node_status.state) + "\r\n"; // @todo 
    message += "raft_role:" + std::to_string(node_status.state) + "\r\n";
    message += "raft_leader_id:" + node_status.leader_id.to_string() + "\r\n";
    message += "raft_current_term:" + std::to_string(node_status.term) + "\r\n";
    // message += "raft_node1:" + node_status.node_info + "\r\n";
    // message += "raft_num_voting_nodes:" + std::to_string(node_status.num_voting_nodes) + "\r\n";
    // message += "raft_num_nodes:" + std::to_string(node_status.num_nodes) + "\r\n";
    // message += "raft_is_voting:" + node_status.is_voting + "\r\n";

    client->AppendString(message);
  } else {
    client->SetRes(CmdRes::kErrOther, "ERR the cmd is not supported");
  }
}

}  // namespace pikiwidb