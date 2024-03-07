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

static void FitLimit(int64_t& count, int64_t& offset, const int64_t size) {
  count = count >= 0 ? count : size;
  offset = (offset >= 0 && offset < size) ? offset : size;
  count = (offset + count < size) ? count : size - offset;
}

int32_t DoScoreStrRange(std::string begin_score, std::string end_score, bool* left_close, bool* right_close,
                        double* min_score, double* max_score) {
  if (!begin_score.empty() && begin_score.at(0) == '(') {
    *left_close = false;
    begin_score.erase(begin_score.begin());
  }
  if (begin_score == "-inf") {
    *min_score = storage::ZSET_SCORE_MIN;
  } else if (begin_score == "inf" || begin_score == "+inf") {
    *min_score = storage::ZSET_SCORE_MAX;
  } else if (pstd::String2d(begin_score.data(), begin_score.size(), min_score) == 0) {
    return -1;
  }

  if (!end_score.empty() && end_score.at(0) == '(') {
    *right_close = false;
    end_score.erase(end_score.begin());
  }
  if (end_score == "+inf" || end_score == "inf") {
    *max_score = storage::ZSET_SCORE_MAX;
  } else if (end_score == "-inf") {
    *max_score = storage::ZSET_SCORE_MIN;
  } else if (pstd::String2d(end_score.data(), end_score.size(), max_score) == 0) {
    return -1;
  }
  return 0;
}

ZAddCmd::ZAddCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZAddCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZAddCmd::DoCmd(PClient* client) {
  size_t argc = client->argv_.size();
  if (argc % 2 == 1) {
    client->SetRes(CmdRes::kSyntaxErr);
    return;
  }
  score_members_.clear();
  double score = 0.0;
  size_t index = 2;
  for (; index < argc; index += 2) {
    if (pstd::String2d(client->argv_[index].data(), client->argv_[index].size(), &score) == 0) {
      client->SetRes(CmdRes::kInvalidFloat);
      return;
    }
    score_members_.push_back({score, client->argv_[index + 1]});
  }
  client->SetKey(client->argv_[1]);
  int32_t count = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->ZAdd(client->Key(), score_members_, &count);
  if (s.ok()) {
    client->AppendInteger(count);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

ZRevrangeCmd::ZRevrangeCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySortedSet) {}

bool ZRevrangeCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRevrangeCmd::DoCmd(PClient* client) {
  std::string key;
  int64_t start = 0;
  int64_t stop = -1;
  bool is_ws = false;
  if (client->argv_.size() == 5 && (strcasecmp(client->argv_[4].data(), "withscores") == 0)) {
    is_ws = true;
  } else if (client->argv_.size() != 4) {
    client->SetRes(CmdRes::kSyntaxErr);
    return;
  }
  if (pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &start) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  if (pstd::String2int(client->argv_[3].data(), client->argv_[3].size(), &stop) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  std::vector<storage::ScoreMember> score_members;
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())
          ->ZRevrange(client->Key(), static_cast<int32_t>(start), static_cast<int32_t>(stop), &score_members);
  if (s.ok() || s.IsNotFound()) {
    if (is_ws) {
      char buf[32];
      int64_t len;
      client->AppendArrayLenUint64(score_members.size() * 2);
      for (const auto& sm : score_members) {
        client->AppendStringLenUint64(sm.member.size());
        client->AppendContent(sm.member);
        len = pstd::D2string(buf, sizeof(buf), sm.score);
        client->AppendStringLen(len);
        client->AppendContent(buf);
      }
    } else {
      client->AppendArrayLenUint64(score_members.size());
      for (const auto& sm : score_members) {
        client->AppendStringLenUint64(sm.member.size());
        client->AppendContent(sm.member);
      }
    }
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

ZRangebyscoreCmd::ZRangebyscoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySortedSet) {}

bool ZRangebyscoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRangebyscoreCmd::DoCmd(PClient* client) {
  double min_score = 0, max_score = 0;
  bool left_close = true, right_close = true, with_scores = false;
  int64_t offset = 0, count = -1;
  int32_t ret = DoScoreStrRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &min_score, &max_score);
  if (ret == -1) {
    client->SetRes(CmdRes::kErrOther, "min or max is not a float");
    return;
  }
  size_t argc = client->argv_.size();
  if (argc >= 5) {
    size_t index = 4;
    while (index < argc) {
      if (strcasecmp(client->argv_[index].data(), "withscores") == 0) {
        with_scores = true;
      } else if (strcasecmp(client->argv_[index].data(), "limit") == 0) {
        if (index + 3 > argc) {
          client->SetRes(CmdRes::kSyntaxErr);
          return;
        }
        index++;
        if (pstd::String2int(client->argv_[index].data(), client->argv_[index].size(), &offset) == 0) {
          client->SetRes(CmdRes::kInvalidInt);
          return;
        }
        index++;
        if (pstd::String2int(client->argv_[index].data(), client->argv_[index].size(), &count) == 0) {
          client->SetRes(CmdRes::kInvalidInt);
          return;
        }
      } else {
        client->SetRes(CmdRes::kSyntaxErr);
        return;
      }
      index++;
    }
  }

  if (min_score == storage::ZSET_SCORE_MAX || max_score == storage::ZSET_SCORE_MIN) {
    client->AppendContent("*0");
    return;
  }
  std::vector<storage::ScoreMember> score_members;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())
                          ->ZRangebyscore(client->Key(), min_score, max_score, left_close, right_close, &score_members);
  if (!s.ok() && !s.IsNotFound()) {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    return;
  }
  FitLimit(count, offset, static_cast<int64_t>(score_members.size()));
  size_t start = offset;
  size_t end = offset + count;
  if (with_scores) {
    char buf[32];
    int64_t len = 0;
    client->AppendArrayLen(count * 2);
    for (; start < end; start++) {
      client->AppendStringLenUint64(score_members[start].member.size());
      client->AppendContent(score_members[start].member);
      len = pstd::D2string(buf, sizeof(buf), score_members[start].score);
      client->AppendStringLen(len);
      client->AppendContent(buf);
    }
  } else {
    client->AppendArrayLen(count);
    for (; start < end; start++) {
      client->AppendStringLenUint64(score_members[start].member.size());
      client->AppendContent(score_members[start].member);
    }
  }
}

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