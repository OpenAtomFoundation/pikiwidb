/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_keys.h"
#include "store.h"

namespace pikiwidb {

DelCmd::DelCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool DelCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void DelCmd::DoCmd(PClient* client) {
  int64_t count = PSTORE.GetBackend()->Del(client->Keys());
  if (count >= 0) {
    client->AppendInteger(count);
  } else {
    client->SetRes(CmdRes::kErrOther, "delete error");
  }
}

ExistsCmd::ExistsCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryKeyspace) {}

bool ExistsCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void ExistsCmd::DoCmd(PClient* client) {
  int64_t count = PSTORE.GetBackend()->Exists(client->Keys());
  if (count >= 0) {
    client->AppendInteger(count);
  } else {
    client->SetRes(CmdRes::kErrOther, "exists internal error");
  }
}

}  // namespace pikiwidb