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
#include "common.h"
#include "pstd/env.h"
#include "store.h"
#include "unbounded_buffer.h"

namespace pikiwidb {

SIsMemberCmd::SIsMemberCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SIsMemberCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
void SIsMemberCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  auto replyNum = 0;  // only change to 1 if ismember . key not exist it is 0
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  if (err == kPErrorOK && value->CastSet()->contains(client->argv_[2])) {
    // only key exist and set has key , set 1

    replyNum = 1;
  }

  client->AppendInteger(replyNum);
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
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      value = PSTORE.SetValue(client->Key(), PObject::CreateSet());
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "sadd cmd error");
      return;
    }
  }
  auto set = value->CastSet();
  const auto oldSize = set->size();
  for (int i = 2; i < client->argv_.size(); ++i) {
    set->insert(client->argv_[i]);
  }
  // new size is bigger than old size , avoid the risk
  client->AppendInteger(set->size() - oldSize);
}

SUnionStoreCmd::SUnionStoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SUnionStoreCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin(), client->argv_.end());
  keys.erase(keys.begin());
  client->SetKey(keys);
  return true;
}

void SUnionStoreCmd::DoCmd(PClient* client) {
  std::unordered_set<std::string> unionSet;
  std::string destKey = client->Keys().at(0);
  std::vector<std::string> keys(client->Keys().begin() + 1, client->Keys().end());

  PObject* value = nullptr;
  for (auto key : keys) {
    PError err = PSTORE.GetValueByType(key, value, kPTypeSet);
    if (err == kPErrorOK) {
      const auto set = value->CastSet();
      auto it = set->cbegin();
      for (; it != set->cend(); ++it) {
        std::string sv(it->data(), it->size());
        if (unionSet.find(sv) == unionSet.end()) {
          unionSet.insert(sv);
        }
      }
    } else if (err != kPErrorNotExist) {
      client->SetRes(CmdRes::kErrOther);
      return;
    }
  }

  PError err = PSTORE.GetValueByType(destKey, value, kPTypeSet);
  if (err == kPErrorOK) {
    auto updateSet = value->CastSet();
    updateSet->clear();
    for (auto it : unionSet) {
      updateSet->emplace(it);
    }
    client->AppendInteger(updateSet->size());
  } else if (err == kPErrorNotExist) {
    value = PSTORE.SetValue(destKey, PObject::CreateSet());
    auto updateSet = value->CastSet();
    for (auto it : unionSet) {
      updateSet->emplace(it);
    }
    client->AppendInteger(updateSet->size());
  } else {
    client->SetRes(CmdRes::kErrOther);
  }
}

SRemCmd::SRemCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SRemCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SRemCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  int retVal = 0;
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      client->AppendInteger(0);
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "srem cmd error");
    }
    return;
  }
  auto unset = value->CastSet();
  const auto oldSize = unset->size();
  for (int i = 2; i < client->argv_.size(); ++i) {
    unset->erase(client->argv_[i]);
  }
  client->AppendInteger(oldSize - unset->size());
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
  if (num_rand == 1) {  // srand 只用返回一个元素即可
    if (err == kPErrorNotExist) {
      client->AppendString("");
      return;
    }
    SRandWithoutCount(client, value);
  } else {
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