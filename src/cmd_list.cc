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

LRangeCmd::LRangeCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryList) {}

bool LRangeCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void LRangeCmd::DoCmd(PClient* client) {
  std::vector<std::string> ret;
  int64_t start_index = 0, end_index = 0;
  if (pstd::String2int(client->argv_[2], &start_index) == 0 || pstd::String2int(client->argv_[3], &end_index) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->LRange(client->Key(), start_index, end_index, &ret);
  if (!s.ok() && !s.IsNotFound()) {
    client->SetRes(CmdRes::kSyntaxErr, "lrange cmd error");
    return;
  }
  client->AppendStringVector(ret);
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
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())->LRem(client->Key(), freq_, client->argv_[3], &reply_num);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(reply_num);
  } else {
    client->SetRes(CmdRes::kErrOther, "lrem cmd error");
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
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->LTrim(client->Key(), start_index, end_index);
  if (s.ok() || s.IsNotFound()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "ltrim cmd error");
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
  const std::string index_str = client->argv_[2];

  if (IsValidNumber(index_str)) {
    int64_t val = 0;
    if (1 != pstd::String2int(index_str, &val)) {
      client->SetRes(CmdRes::kErrOther, "lset cmd error");  // this will not happend in normal case
      return;
    }
    storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->LSet(client->Key(), val, client->argv_[3]);
    if (s.ok()) {
      client->SetRes(CmdRes::kOK);
    } else if (s.IsNotFound()) {
      client->SetRes(CmdRes::kNotFound);
    } else if (s.IsCorruption()) {
      client->SetRes(CmdRes::kOutOfRange);
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "lset cmd error");  // just a safeguard
    }
  } else {
    client->SetRes(CmdRes::kInvalidInt);
  }
}

LInsertCmd::LInsertCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}

bool LInsertCmd::DoInitial(PClient* client) {
  if (!pstd::StringEqualCaseInsensitive(client->argv_[2], "BEFORE") &&
      !pstd::StringEqualCaseInsensitive(client->argv_[2], "AFTER")) {
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void LInsertCmd::DoCmd(PClient* client) {
  int64_t ret = 0;
  storage ::BeforeOrAfter before_or_after = storage::Before;
  if (pstd::StringEqualCaseInsensitive(client->argv_[2], "AFTER")) {
    before_or_after = storage::After;
  }
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())
                          ->LInsert(client->Key(), before_or_after, client->argv_[3], client->argv_[4], &ret);
  if (!s.ok() && s.IsNotFound()) {
    client->SetRes(CmdRes::kSyntaxErr, "lset cmd error");  // just a safeguard
    return;
  }
  client->AppendInteger(ret);
}
}  // namespace pikiwidb
