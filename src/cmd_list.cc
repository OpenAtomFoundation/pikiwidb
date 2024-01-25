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
LSetCmd::LSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}
bool LSetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
void LSetCmd::DoCmd(PClient* client) {
  // isVaildNumber ensures that the string is in decimal format,
  // while strtol ensures that the string is within the range of long type
  std::string& index_str = client->argv_[2];

  if (IsValidNumber(index_str)) {
    int64_t val = 0;
    if (1 != pstd::String2int(index_str, &val)) {
      client->SetRes(CmdRes::kErrOther, "lset cmd error");  // this will not happend in normal case
      return;
    }
    storage::Status s = PSTORE.GetBackend()->LSet(client->Key(), val, client->argv_[3]);
    if (s.ok()) {
      client->SetRes(CmdRes::kOK);
    } else if (s.IsNotFound()) {
      client->SetRes(CmdRes::kNotFound);
    } else if (s.IsCorruption()) {
      client->SetRes(CmdRes::kOutOfRange);
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "lset cmd error");  // just a safeguard
    };
  } else {
    client->SetRes(CmdRes::kInvalidInt);
  }
}

}  // namespace pikiwidb
