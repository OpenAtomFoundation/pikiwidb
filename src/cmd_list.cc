/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_list.h"

#include <pstd_string.h>

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
  storage::Status s = PSTORE.GetBackend()->LPush(client->Key(), list_values, &reply_num);
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
  storage::Status s = PSTORE.GetBackend()->RPush(client->Key(), list_values, &reply_num);
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
  storage::Status s = PSTORE.GetBackend()->RPop(client->Key(), 1, &elements);
  if (s.ok()) {
    client->AppendString(elements[0]);
  } else if (s.IsNotFound()) {
    client->AppendStringLen(-1);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "rpop cmd error");
  }
}
LTrimCmd::LTrimCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}
bool LTrimCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);

  return true;
}
void LTrimCmd::DoCmd(PClient* client) {
  int64_t start_index = 0, end_index = 0;

  if (pstd::String2int(client->argv_[2], &start_index) == 0 || pstd::String2int(client->argv_[3], &end_index) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  storage::Status s = PSTORE.GetBackend()->LTrim(client->Key(), start_index, end_index);
  if (s.ok() || s.IsNotFound()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "ltrim cmd error");
  }
}
}  // namespace pikiwidb
