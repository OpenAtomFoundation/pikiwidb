/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_set.h"

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

SInter::SInter(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SInter::DoInitial(PClient* client) {
  std::vector keys(client->argv_.begin() + 1, client->argv_.end());

  client->SetKey(keys);
  return true;
}
// todo ?有这样的通用方法卸载common中还是这里
PSet getInterSet(const PSET& set1, const PSET& set2) {
  PSet interSet;
  interSet.reserve(std::min(set1->size(), set2->size()));
  auto set1P = &set1;
  auto set2P = &set1;
  if (set1->size() > set2->size()) {
    std::swap(set1P, set2P);
  }
  for (const auto& item1 : set1) {
    if (set2->contains(item1)) {
      interSet.emplace(item1);
    }
  }
  return interSet;
}
/**
 *  两个set求交集，时间复杂度为O（小的set的长度）
 * \brief 如果这里的key很多的话，那么可以考虑对短的set先处理
 * 但是下层的rocksDB读放大的问题，因此还是不排序了？
 * 目前是不排序的状态
 * \param client
 */
void SInter::DoCmd(PClient* client) {
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
  for(const auto &member:firstSet) {
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