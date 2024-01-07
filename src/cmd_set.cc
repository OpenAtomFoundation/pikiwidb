/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_set.h"
#include <memory>
#include <utility>
#include "store.h"

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
SInterCmd::SInterCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SInterCmd::DoInitial(PClient* client) {
  std::vector keys(client->argv_.begin() + 1, client->argv_.end());

  client->SetKey(keys);
  return true;
}
// todo ?有这样的通用方法卸载common中还是这里
// tobe delete
// PSet getInterSet(const PSET& set1, const PSET& set2) {
//   PSet interSet;
//   interSet.reserve(std::min(set1->size(), set2->size()));
//   auto set1P = &set1;
//   auto set2P = &set1;
//   if (set1->size() > set2->size()) {
//     std::swap(set1P, set2P);
//   }
//   for (const auto& item1 : *set1) {
//     if (set2->contains(item1)) {
//       interSet.emplace(item1);
//     }
//   }
//   return interSet;
// }

void SInterCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  std::vector<std::string> resVt;
  std::string setKey = client->Keys().at(0);
  PError err = PSTORE.GetValueByType(setKey, value, kPTypeSet);
  if (err == kPErrorNotExist) {
    client->AppendStringVector(resVt);
    return;
  }
  PSET firstSet = value->CastSet();
  bool reliable{};
  for(const auto &member: *firstSet) {
    reliable = true;
    for (int i = 1; i < client->Keys().size(); ++i) {
      setKey = client->Keys().at(i);
      err = PSTORE.GetValueByType(setKey, value, kPTypeSet);
      if (err == kPErrorNotExist) {
        client->AppendStringVector(resVt);
        return;
      }
      if(!value->CastSet()->contains(member)) {
        reliable = false;
        break;
      }
    }
    if (reliable == true) {
      resVt.emplace_back(member);
    }
  }

  client->AppendStringVector(resVt);
}

}  // namespace pikiwidb