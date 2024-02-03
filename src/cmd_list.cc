/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_list.h"
#include "pstd_string.h"
#include "store.h"

namespace pikiwidb {

LPushCmd::LPushCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}

bool LPushCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void LPushCmd::DoCmd(PClient* client) {
  std::vector<std::string> list_values(client->argv_.begin() + 2, client->argv_.end());
  uint64_t reply_num = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->LPush(client->Key(), list_values, &reply_num);
  if (s.ok()) {
    client->AppendInteger(reply_num);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "lpush cmd error");
  }
}

RPushCmd::RPushCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}

bool RPushCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void RPushCmd::DoCmd(PClient* client) {
  std::vector<std::string> list_values(client->argv_.begin() + 2, client->argv_.end());
  uint64_t reply_num = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->RPush(client->Key(), list_values, &reply_num);
  if (s.ok()) {
    client->AppendInteger(reply_num);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "rpush cmd error");
  }
}

RPopCmd::RPopCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}

bool RPopCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void RPopCmd::DoCmd(PClient* client) {
  std::vector<std::string> elements;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->RPop(client->Key(), 1, &elements);
  if (s.ok()) {
    client->AppendString(elements[0]);
  } else if (s.IsNotFound()) {
    client->AppendStringLen(-1);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "rpop cmd error");
  }
}

LRemCmd::LRemCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}

bool LRemCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void LRemCmd::DoCmd(PClient* client) {
  int64_t freq_ = 0;
  std::string count = client->argv_[2];
  if (pstd::String2int(count, &freq_) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  uint64_t reply_num = 0;
  storage::Status s = PSTORE.GetBackend()->LRem(client->Key(), freq_, client->argv_[3], &reply_num);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(reply_num);
  } else {
    client->SetRes(CmdRes::kErrOther, "lrem cmd error");
  }
}

}  // namespace pikiwidb
