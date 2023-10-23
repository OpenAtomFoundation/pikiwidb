/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_admin.h"

namespace pikiwidb {

CmdConfig::CmdConfig(const std::string& name, int arity) : BaseCmdGroup(name, CmdFlagsAdmin, AclCategoryAdmin) {
  subCmd_ = {"set", "get"};
}

bool CmdConfig::HasSubCommand() const { return true; }

CmdConfigGet::CmdConfigGet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin | CmdFlagsWrite, AclCategoryAdmin) {}

bool CmdConfigGet::DoInitial(CmdContext& ctx) { return true; }

void CmdConfigGet::DoCmd(CmdContext& ctx) { ctx.AppendString("config cmd in development"); }

CmdConfigSet::CmdConfigSet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsAdmin, AclCategoryAdmin) {}

bool CmdConfigSet::DoInitial(CmdContext& ctx) { return true; }

void CmdConfigSet::DoCmd(CmdContext& ctx) { ctx.AppendString("config cmd in development"); }

}  // namespace pikiwidb