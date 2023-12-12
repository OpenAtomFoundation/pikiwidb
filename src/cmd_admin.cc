/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_admin.h"
#include "store.h"

namespace pikiwidb {

CmdConfig::CmdConfig(const std::string& name, int arity) : BaseCmdGroup(name, CmdFlagsAdmin, AclCategoryAdmin) {}

bool CmdConfig::HasSubCommand() const { return true; }

CmdConfigGet::CmdConfigGet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin | CmdFlagsWrite, AclCategoryAdmin) {}

bool CmdConfigGet::DoInitial(PClient* client) { return true; }

void CmdConfigGet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

CmdConfigSet::CmdConfigSet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin, AclCategoryAdmin) {}

bool CmdConfigSet::DoInitial(PClient* client) { return true; }

void CmdConfigSet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

FlushdbCmd::FlushdbCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin | CmdFlagsWrite, AclCategoryWrite | AclCategoryAdmin) {}

bool FlushdbCmd::DoInitial(PClient* client) { return true; }

void FlushdbCmd::DoCmd(PClient* client) {
  PSTORE.dirty_ += PSTORE.DBSize();
  PSTORE.ClearCurrentDB();
  Propagate(PSTORE.GetDB(), std::vector<PString>{"flushdb"});
  client->SetRes(CmdRes::kOk);
}

FlushallCmd::FlushallCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin | CmdFlagsWrite, AclCategoryWrite | AclCategoryAdmin) {}

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
  client->SetRes(CmdRes::kOk);
}

}  // namespace pikiwidb