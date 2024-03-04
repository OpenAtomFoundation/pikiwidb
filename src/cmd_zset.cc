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

ZRevRangeByScoreCmd::ZRevRangeByScoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZRevRangeByScoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRevRangeByScoreCmd::DoCmd(PClient* client) {
  double minScore, maxScore;
  if (pstd::String2d(client->argv_[2].c_str(), client->argv_[2].size(), &minScore) == 0 ||
      pstd::String2d(client->argv_[3].c_str(), client->argv_[3].size(), &maxScore) == 0) {
    client->SetRes(CmdRes::kInvalidFloat);
    return;
  }

  int64_t count, offset;
  if (pstd::String2int(client->argv_[4].c_str(), &count) == 0 ||
      pstd::String2int(client->argv_[5].c_str(), &offset) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  int32_t ret = 0;
  std::vector<storage::ScoreMember> scoreMembers;
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())
          ->ZRevrangebyscore(client->Key(), minScore, maxScore,
                             /* left_close = */ false, /* right_close = */ false, count, offset, &scoreMembers);

  if (s.ok()) {
    for (const auto& scoreMember : scoreMembers) {
      client->AppendString(scoreMember.member);
    }
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  return;
}

}  // namespace pikiwidb