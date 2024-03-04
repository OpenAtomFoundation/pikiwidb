/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_zset.h"

#include <memory>

#include "pstd/pstd_string.h"
#include "store.h"

namespace pikiwidb {

ZRemRangeByRankCmd::ZRemRangeByRankCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool ZRemRangeByRankCmd::DoInitial(pikiwidb::PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRemRangeByRankCmd::DoCmd(pikiwidb::PClient* client) {
  int32_t ret = 0;

  int32_t start = 0;
  int32_t end = 0;

  if (pstd::String2int(client->argv_[2], &start) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  if (pstd::String2int(client->argv_[3], &end) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  storage::Status s;
  s = PSTORE.GetBackend(client->GetCurrentDB())->ZRemrangebyrank(client->Key(), start, end, &ret);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(ret);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

}  // namespace pikiwidb