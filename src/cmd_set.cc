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
  int32_t replyNum{};  // only change to 1 if ismember . key not exist it is 0
  PSTORE.GetBackend()->SIsmember(client->Key(), client->argv_[2], &replyNum);

  // todo delete if test success
  // PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  // if (err == kPErrorOK && value->CastSet()->contains(client->argv_[2])) {
  //   // only key exist and set has key , set 1
  //
  //   replyNum = 1;
  // }

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
  const std::vector<std::string> members(client->argv_.begin() + 2, client->argv_.end());
  int32_t ret{};
  storage::Status s = PSTORE.GetBackend()->SAdd(client->Key(), members, &ret);
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
  std::string destKey = client->Keys().at(0);
  std::vector<std::string> keys(client->Keys().begin() + 1, client->Keys().end());
  std::vector<std::string> value_to_dest;
  int32_t ret{};
  storage::Status s = PSTORE.GetBackend()->SUnionstore(client->Keys().at(0), keys, value_to_dest, &ret);
  if (!s.ok()) {
    client->SetRes(CmdRes::kSyntaxErr, "sunionstore cmd error");
  }
  client->AppendInteger(ret);
  // //todo to delete if test success
  // std::unordered_set<std::string> unionSet;
  // std::string destKey = client->Keys().at(0);
  // std::vector<std::string> keys(client->Keys().begin() + 1, client->Keys().end());
  //
  // PObject* value = nullptr;
  // for (auto key : keys) {
  //   PError err = PSTORE.GetValueByType(key, value, kPTypeSet);
  //   if (err == kPErrorOK) {
  //     const auto set = value->CastSet();
  //     auto it = set->cbegin();
  //     for (; it != set->cend(); ++it) {
  //       std::string sv(it->data(), it->size());
  //       if (unionSet.find(sv) == unionSet.end()) {
  //         unionSet.insert(sv);
  //       }
  //     }
  //   } else if (err != kPErrorNotExist) {
  //     client->SetRes(CmdRes::kErrOther);
  //     return;
  //   }
  // }
  //
  // PError err = PSTORE.GetValueByType(destKey, value, kPTypeSet);
  // if (err == kPErrorOK) {
  //   auto updateSet = value->CastSet();
  //   updateSet->clear();
  //   for (auto it : unionSet) {
  //     updateSet->emplace(it);
  //   }
  //   client->AppendInteger(updateSet->size());
  // } else if (err == kPErrorNotExist) {
  //   value = PSTORE.SetValue(destKey, PObject::CreateSet());
  //   auto updateSet = value->CastSet();
  //   for (auto it : unionSet) {
  //     updateSet->emplace(it);
  //   }
  //   client->AppendInteger(updateSet->size());
  // } else {
  //   client->SetRes(CmdRes::kErrOther);
  // }
}
SInterCmd::SInterCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SInterCmd::DoInitial(PClient* client) {
  std::vector keys(client->argv_.begin() + 1, client->argv_.end());

  client->SetKey(keys);
  return true;
}

void SInterCmd::DoCmd(PClient* client) {
  std::vector<std::string> resVt;
  storage::Status s = PSTORE.GetBackend()->SInter(client->Keys(), &resVt);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "sinter cmd error");
  }
  client->AppendStringVector(resVt);
}

SRemCmd::SRemCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SRemCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SRemCmd::DoCmd(PClient* client) {
  std::vector<std::string> toDeleteMembers(client->argv_.begin() + 2, client->argv_.end());
  int32_t replyNum{};
  storage::Status s = PSTORE.GetBackend()->SRem(client->Key(), toDeleteMembers, &replyNum);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "srem cmd error");
  }
  client->AppendInteger(replyNum);

  return;
  // todo delete if test success
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

SUnionCmd::SUnionCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SUnionCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void SUnionCmd::DoCmd(PClient* client) {
  std::vector<std::string> resVt;
  storage::Status s = PSTORE.GetBackend()->SUnion(client->Keys(), &resVt);
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, "sunion cmd error");
  }
  client->AppendStringVector(resVt);

  return;
  // todo delete if test success
  std::unordered_set<std::string> unionSet;
  for (auto key : client->Keys()) {
    PObject* value = nullptr;
    PError err = PSTORE.GetValueByType(key, value, kPTypeSet);
    if (err == kPErrorOK) {
      const auto set = value->CastSet();
      for (const auto& it : *set) {
        unionSet.insert(it);
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