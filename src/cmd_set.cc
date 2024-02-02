/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_set.h"
#include <memory>
#include <random>
#include <utility>
#include "pstd/env.h"
#include "store.h"

namespace pikiwidb {

SIsMemberCmd::SIsMemberCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SIsMemberCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
void SIsMemberCmd::DoCmd(PClient* client) {
  int32_t reply_Num = 0;  // only change to 1 if ismember . key not exist it is 0
  PSTORE.GetBackend(client->GetCurrentDB())->SIsmember(client->Key(), client->argv_[2], &reply_Num);

  client->AppendInteger(reply_Num);
}

SAddCmd::SAddCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SAddCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
// Integer reply: the number of elements that were added to the set,
// not including all the elements already present in the set.
void SAddCmd::DoCmd(PClient* client) {
  const std::vector<std::string> members(client->argv_.begin() + 2, client->argv_.end());
  int32_t ret = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->SAdd(client->Key(), members, &ret);
  if (s.ok()) {
    client->AppendInteger(ret);
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "sadd cmd error");
  }
}

SUnionStoreCmd::SUnionStoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SUnionStoreCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void SUnionStoreCmd::DoCmd(PClient* client) {
  std::vector<std::string> keys(client->Keys().begin() + 1, client->Keys().end());
  std::vector<std::string> value_to_dest;
  int32_t ret = 0;
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())->SUnionstore(client->Keys().at(0), keys, value_to_dest, &ret);
  if (!s.ok()) {
    client->SetRes(CmdRes::kSyntaxErr, "sunionstore cmd error");
  }
  client->AppendInteger(ret);
}
SInterCmd::SInterCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SInterCmd::DoInitial(PClient* client) {
  std::vector keys(client->argv_.begin() + 1, client->argv_.end());

  client->SetKey(keys);
  return true;
}

void SInterCmd::DoCmd(PClient* client) {
  std::vector<std::string> res_vt;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->SInter(client->Keys(), &res_vt);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "sinter cmd error");
    return;
  }
  client->AppendStringVector(res_vt);
}

SRemCmd::SRemCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SRemCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SRemCmd::DoCmd(PClient* client) {
  std::vector<std::string> to_delete_members(client->argv_.begin() + 2, client->argv_.end());
  int32_t reply_num = 0;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->SRem(client->Key(), to_delete_members, &reply_num);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "srem cmd error");
  }
  client->AppendInteger(reply_num);
}

SUnionCmd::SUnionCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SUnionCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void SUnionCmd::DoCmd(PClient* client) {
  std::vector<std::string> res_vt;
  storage::Status s = PSTORE.GetBackend(client->GetCurrentDB())->SUnion(client->Keys(), &res_vt);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "sunion cmd error");
  }
  client->AppendStringVector(res_vt);
}

SInterStoreCmd::SInterStoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SInterStoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SInterStoreCmd::DoCmd(PClient* client) {
  std::vector<std::string> value_to_dest;
  int32_t reply_num = 0;

  std::vector<std::string> inter_keys(client->argv_.begin() + 2, client->argv_.end());
  storage::Status s =
      PSTORE.GetBackend(client->GetCurrentDB())->SInterstore(client->Key(), inter_keys, value_to_dest, &reply_num);
  if (!s.ok()) {
    client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
    return;
  }
  client->AppendInteger(reply_num);
}

SRandMemberCmd::SRandMemberCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SRandMemberCmd::DoInitial(PClient* client) {
  if (client->argv_.size() > 3) {
    client->SetRes(CmdRes::kWrongNum, client->CmdName());
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void SRandMemberCmd::DoCmd(PClient* client) {
  int num_rand = 1;
  if (client->argv_.size() > 2) {
    try {
      num_rand = std::stoi(client->argv_[2]);
    } catch (const std::invalid_argument& e) {
      client->SetRes(CmdRes::kInvalidBitInt, "srandmember cmd error");
      return;
    }
  }
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  if (err != kPErrorOK && err != kPErrorNotExist) {
    client->SetRes(CmdRes::kSyntaxErr, "srem cmd error");
  }
  if (client->argv_.size() == 2) {  // srand 只用返回一个元素即可
    if (err == kPErrorNotExist) {
      client->AppendString("");
      return;
    }
    SRandWithoutCount(client, value);
  } else if(client->argv_.size() == 3) {
    if (err == kPErrorNotExist) {
      client->AppendArrayLen(0);
      return;
    }
    SRandWithCount(client, value, num_rand);
  }
}

void SRandMemberCmd::SRandWithoutCount(PClient* client, PObject* value) {
  auto last_seed = pstd::NowMicros();
  std::default_random_engine engine;
  engine.seed(last_seed);
  PSET value_set = value->CastSet();
  auto pos = static_cast<uint32_t>(engine() % (value_set->size()));
  auto it = value_set->begin();
  // The impl. of redis is to randomly find a non-empty hash bucket and
  // then find a random element in the non-empty bucket (further improvement).
  std::advance(it, pos);
  if (it != value_set->end()) {
    client->AppendString(*it);
  } else {
    client->SetRes(CmdRes::kErrOther);
  }
}

void SRandMemberCmd::SRandWithCount(PClient* client, PObject* value, int num_rand) {
  auto last_seed = pstd::NowMicros();
  std::default_random_engine engine;
  engine.seed(last_seed);
  PSET value_set = value->CastSet();
  uint32_t size = value_set->size();
  if (size == 0) {
    client->AppendStringVector({});
    return;
  }
  std::vector<uint32_t> targets;
  std::unordered_set<uint32_t> unique;
  if (num_rand < 0) {
    num_rand = -num_rand;
    while (targets.size() < num_rand) {
      engine.seed(last_seed);
      last_seed = static_cast<int64_t>(engine());
      targets.push_back(static_cast<uint32_t>(engine() % size));
    }
  } else {
    num_rand = num_rand <= size ? num_rand : size;
    while (targets.size() < num_rand) {
      engine.seed(last_seed);
      last_seed = static_cast<int64_t>(engine());
      auto pos = static_cast<int32_t>(engine() % size);
      if (unique.find(pos) == unique.end()) {
        unique.insert(pos);
        targets.push_back(pos);
      }
    }
  }
  std::sort(targets.begin(), targets.end());
  // pika code is to generate all random numbers in one iteration, so sort was used.
  // Further optim. will be considered (redis generate different ways under diff. conditions)
  std::vector<PString> members;
  auto iter = value_set->begin();
  uint32_t cur_pos = 0;
  for (auto index = 0; index < targets.size(); index++) {
    while (cur_pos < targets[index]) {
      cur_pos++;
      iter++;
    }
    members.emplace_back(*iter);
  }
  std::shuffle(members.begin(), members.end(), engine);
  client->AppendStringVector(members);
}
}  // namespace pikiwidb
