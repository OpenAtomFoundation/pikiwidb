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
  auto resPair = set->emplace(client->argv_[2]);
  if (resPair.second) {
    client->AppendInteger(1);
  } else {
    client->AppendInteger(0);
  }
}

SUnionCmd::SUnionCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SUnionCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin(), client->argv_.end());
  keys.erase(keys.begin());
  client->SetKey(keys);
  return true;
}

void SUnionCmd::DoCmd(PClient* client) {
  std::unordered_set<std::string> unionSet;
  for (auto key : client->Keys()) {
    PObject* value = nullptr;
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
  client->AppendArrayLenUint64(unionSet.size());
  for (const auto& member : unionSet) {
    client->AppendStringLenUint64(member.size());
    client->AppendContent(member);
  }
}
}  // namespace pikiwidb