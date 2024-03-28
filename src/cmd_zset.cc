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

static int32_t DoMemberRange(const std::string& raw_min_member, const std::string& raw_max_member, bool* left_close,
                             bool* right_close, std::string* min_member, std::string* max_member) {
  if (raw_min_member == "-") {
    *min_member = "-";
  } else if (raw_min_member == "+") {
    *min_member = "+";
  } else {
    if (!raw_min_member.empty() && raw_min_member.at(0) == '(') {
      *left_close = false;
    } else if (!raw_min_member.empty() && raw_min_member.at(0) == '[') {
      *left_close = true;
    } else {
      return -1;
    }
    min_member->assign(raw_min_member.begin() + 1, raw_min_member.end());
  }

  if (raw_max_member == "+") {
    *max_member = "+";
  } else if (raw_max_member == "-") {
    *max_member = "-";
  } else {
    if (!raw_max_member.empty() && raw_max_member.at(0) == '(') {
      *right_close = false;
    } else if (!raw_max_member.empty() && raw_max_member.at(0) == '[') {
      *right_close = true;
    } else {
      return -1;
    }
    max_member->assign(raw_max_member.begin() + 1, raw_max_member.end());
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
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->ZAdd(client->Key(), score_members_, &count);
  if (s.ok()) {
    client->AppendInteger(count);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kmultikey);
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
          ->GetStorage()
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
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kmultikey);
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
                          ->GetStorage()
                          ->ZRangebyscore(client->Key(), min_score, max_score, left_close, right_close, &score_members);

  if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kmultikey);
    return;
  }
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

ZCardCmd::ZCardCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySortedSet) {}

bool ZCardCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZCardCmd::DoCmd(PClient* client) {
  int32_t reply_Num = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->ZCard(client->Key(), &reply_Num);
  if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kmultikey);
    return;
  }
  if (!s.ok()) {
    client->SetRes(CmdRes::kSyntaxErr, "ZCard cmd error");
    return;
  }
  client->AppendInteger(reply_Num);
}

ZRemrangebyrankCmd::ZRemrangebyrankCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool ZRemrangebyrankCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRemrangebyrankCmd::DoCmd(PClient* client) {
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
  s = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->ZRemrangebyrank(client->Key(), start, end, &ret);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(ret);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

ZRangeCmd::ZRangeCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySortedSet) {}

bool ZRangeCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRangeCmd::DoCmd(PClient* client) {
  double start = 0;
  double stop = 0;
  int64_t count = -1;
  int64_t offset = 0;
  bool with_scores = false;
  bool by_score = false;
  bool by_lex = false;
  bool left_close = false;
  bool right_close = false;
  bool is_rev = false;
  size_t argc = client->argv_.size();
  if (argc >= 5) {
    size_t index = 4;
    while (index < argc) {
      if (strcasecmp(client->argv_[index].data(), "byscore") == 0) {
        by_score = true;
      } else if (strcasecmp(client->argv_[index].data(), "bylex") == 0) {
        by_lex = true;
      } else if (strcasecmp(client->argv_[index].data(), "rev") == 0) {
        is_rev = true;
      } else if (strcasecmp(client->argv_[index].data(), "withscores") == 0) {
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
  if (by_score && by_lex) {
    client->SetRes(CmdRes::kSyntaxErr);
    return;
  }

  int32_t ret = 0;
  std::string lex_min;
  std::string lex_max;
  if (by_lex) {
    ret = DoMemberRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &lex_min, &lex_max);
    if (ret == -1) {
      client->SetRes(CmdRes::kErrOther, "min or max not valid string range item");
      return;
    }
  } else {
    ret = DoScoreStrRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &start, &stop);
    if (ret == -1) {
      client->SetRes(CmdRes::kErrOther, "start or stop is not a float");
      return;
    }
  }

  std::vector<storage::ScoreMember> score_members;
  std::vector<std::string> lex_members;
  storage::Status s;
  if (!is_rev) {
    if (by_score) {
      s = PSTORE.GetBackend(client->GetCurrentDB())
              ->GetStorage()
              ->ZRangebyscore(client->Key(), start, stop, left_close, right_close, &score_members);
    } else if (by_lex) {
      s = PSTORE.GetBackend(client->GetCurrentDB())
              ->GetStorage()
              ->ZRangebylex(client->Key(), lex_min, lex_max, left_close, right_close, &lex_members);
    } else {
      s = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->ZRange(client->Key(), start, stop, &score_members);
    }
  } else {
    if (by_score) {
      s = PSTORE.GetBackend(client->GetCurrentDB())
              ->GetStorage()
              ->ZRevrangebyscore(client->Key(), start, stop, left_close, right_close, &score_members);
    } else if (by_lex) {
      s = PSTORE.GetBackend(client->GetCurrentDB())
              ->GetStorage()
              ->ZRangebylex(client->Key(), lex_min, lex_max, left_close, right_close, &lex_members);
    } else {
      s = PSTORE.GetBackend(client->GetCurrentDB())
              ->GetStorage()
              ->ZRevrange(client->Key(), start, stop, &score_members);
    }
  }
  if (!s.ok() && !s.IsNotFound()) {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    return;
  }
  FitLimit(count, offset, static_cast<int64_t>(score_members.size()));
  size_t m_start = offset;
  size_t m_end = offset + count;
  if (by_lex) {
    if (with_scores) {
      client->SetRes(CmdRes::kSyntaxErr, "by lex not support with scores");
    } else {
      client->AppendArrayLen(count);
      for (; m_start < m_end; m_start++) {
        client->AppendContent(lex_members[m_start]);
      }
    }
  } else {
    if (with_scores) {
      char buf[32];
      int64_t len = 0;
      client->AppendArrayLen(count * 2);
      for (; m_start < m_end; m_start++) {
        client->AppendStringLenUint64(score_members[m_start].member.size());
        client->AppendContent(score_members[m_start].member);
        len = pstd::D2string(buf, sizeof(buf), score_members[m_start].score);
        client->AppendStringLen(len);
        client->AppendContent(buf);
      }
    } else {
      client->AppendArrayLen(count);
      for (; m_start < m_end; m_start++) {
        client->AppendStringLenUint64(score_members[m_start].member.size());
        client->AppendContent(score_members[m_start].member);
      }
    }
  }
}

ZScoreCmd::ZScoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

bool ZScoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZScoreCmd::DoCmd(PClient* client) {
  double score = 0;

  storage::Status s;
  s = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->ZScore(client->Key(), client->argv_[2], &score);
  if (s.ok() || s.IsNotFound()) {
    client->AppendString(std::to_string(score));
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

ZRevrangebyscoreCmd::ZRevrangebyscoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZRevrangebyscoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRevrangebyscoreCmd::DoCmd(PClient* client) {
  double min_score = 0;
  double max_score = 0;
  bool right_close = true;
  bool left_close = true;
  bool with_scores = false;
  int64_t offset = 0, count = -1;
  int32_t ret = DoScoreStrRange(client->argv_[3], client->argv_[2], &left_close, &right_close, &min_score, &max_score);
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
                          ->GetStorage()
                          ->ZRevrangebyscore(client->Key(), min_score, max_score, left_close, right_close, count,
                                             offset, &score_members);
  if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kmultikey);
  }
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

ZRangebylexCmd::ZRangebylexCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZRangebylexCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRangebylexCmd::DoCmd(PClient* client) {
  if (strcasecmp(client->argv_[2].data(), "+") == 0 || strcasecmp(client->argv_[3].data(), "-") == 0) {
    client->AppendContent("*0");
  }

  size_t argc = client->argv_.size();
  int64_t count = -1;
  int64_t offset = 0;
  bool left_close = true;
  bool right_close = true;
  if (argc == 7 && strcasecmp(client->argv_[4].data(), "limit") == 0) {
    if (pstd::String2int(client->argv_[5].data(), client->argv_[5].size(), &offset) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }
    if (pstd::String2int(client->argv_[6].data(), client->argv_[6].size(), &count) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }
  } else if (argc == 4) {
  } else {
    client->SetRes(CmdRes::kSyntaxErr);
    return;
  }

  std::string min_member;
  std::string max_member;
  int32_t ret = DoMemberRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &min_member, &max_member);
  if (ret == -1) {
    client->SetRes(CmdRes::kErrOther, "min or max not valid string range item");
    return;
  }
  std::vector<std::string> members;
  storage::Status s;
  s = PSTORE.GetBackend(client->GetCurrentDB())
          ->GetStorage()
          ->ZRangebylex(client->Key(), min_member, max_member, left_close, right_close, &members);
  if (!s.ok() && !s.IsNotFound()) {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    return;
  }

  FitLimit(count, offset, static_cast<int64_t>(members.size()));
  size_t index = offset;
  size_t end = offset + count;

  client->AppendArrayLen(static_cast<int64_t>(members.size()));
  for (; index < end; index++) {
    client->AppendStringLenUint64(members[index].size());
    client->AppendContent(members[index]);
  }
}

ZRevrangebylexCmd::ZRevrangebylexCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZRevrangebylexCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRevrangebylexCmd::DoCmd(PClient* client) {
  if (strcasecmp(client->argv_[2].data(), "+") == 0 || strcasecmp(client->argv_[3].data(), "-") == 0) {
    client->AppendContent("*0");
  }

  size_t argc = client->argv_.size();
  int64_t count = -1;
  int64_t offset = 0;
  bool left_close = true;
  bool right_close = true;
  if (argc == 7 && strcasecmp(client->argv_[4].data(), "limit") == 0) {
    if (pstd::String2int(client->argv_[5].data(), client->argv_[5].size(), &offset) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }
    if (pstd::String2int(client->argv_[6].data(), client->argv_[6].size(), &count) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }
  } else if (argc == 4) {
  } else {
    client->SetRes(CmdRes::kSyntaxErr);
    return;
  }

  std::string min_member;
  std::string max_member;
  int32_t ret = DoMemberRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &min_member, &max_member);
  std::vector<std::string> members;
  storage::Status s;
  s = PSTORE.GetBackend(client->GetCurrentDB())
          ->GetStorage()
          ->ZRangebylex(client->Key(), min_member, max_member, left_close, right_close, &members);
  if (!s.ok() && !s.IsNotFound()) {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    return;
  }

  FitLimit(count, offset, static_cast<int64_t>(members.size()));
  size_t index = offset + count - 1;
  size_t start = offset;
  client->AppendArrayLen(static_cast<int64_t>(members.size()));
  for (; index >= start; index--) {
    client->AppendStringLenUint64(members[index].size());
    client->AppendContent(members[index]);
  }
}

ZRemrangebyscoreCmd::ZRemrangebyscoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySortedSet) {}

bool ZRemrangebyscoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ZRemrangebyscoreCmd::DoCmd(PClient* client) {
  double min_score = 0;
  double max_score = 0;
  bool left_close = true;
  bool right_close = true;
  int32_t ret = DoScoreStrRange(client->argv_[2], client->argv_[3], &left_close, &right_close, &min_score, &max_score);
  if (ret == -1) {
    client->SetRes(CmdRes::kErrOther, "min or max is not a float");
    return;
  }

  int32_t s_ret = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())
                          ->GetStorage()
                          ->ZRemrangebyscore(client->Key(), min_score, max_score, left_close, right_close, &s_ret);
  if (s.ok()) {
    client->AppendInteger(s_ret);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

}  // namespace pikiwidb