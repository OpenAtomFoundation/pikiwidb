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
SInterStoreCmd::SInterStoreCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategorySet) {}

bool SInterStoreCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
void SInterStoreCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err;

  // 逻辑：像sinter的逻辑一样获取每一个key，只要有一个key不存在，则返回0
  // 注意，如果本来存在的key被清空了，那么就删除这个key
  // 对于key本来就不存在的情况下的两种方案：1.先生成key，好处：统一写法。坏处：可能会有生成随后就删除的情况，导致浪费；2.先统计结果，再生成key，好处：不会浪费，坏处：中间结果需要保存
  // 现在选择方案1



  //check all value set is exist or not
  bool reliable = true;
  for (int i = 2; i < client->argv_.size(); ++i) {
    err = PSTORE.GetValueByType(client->argv_[i], value, kPTypeSet);
    if (err!= kPErrorOK) {
      if (err != kPErrorNotExist) {
        client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
      }
      reliable = false;
      break;
    }
  }
  if(!reliable) {
    // some value set is not exist, so return
    err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
    if(err!=kPErrorOK && err != kPErrorNotExist  ) {
      client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
    } else {
      //delete key like DelCmd
      if(PSTORE.DeleteKey(client->Key())) {
        PSTORE.ClearExpire(client->Key());
      }
      client->AppendInteger(0);
    }
    return ;
  }

  err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      value = PSTORE.SetValue(client->Key(), PObject::CreateSet());
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
      return;
    }
  }
  auto desSet = value->CastSet();
  desSet->clear();

  // end check , all value set is exist
  err = PSTORE.GetValueByType(client->argv_[2],value,kPTypeSet); //get girst value key

  PSET firstSet = value->CastSet();
  for(const auto &member:*firstSet) {
    reliable = true; //here reliable 用于检查对于一个string，是不是所有的value set 都存在
    for(int i = 3;i<client->argv_.size();++i) { //start from second value key
      err = PSTORE.GetValueByType(client->argv_[i],value,kPTypeSet);
      if (err!= kPErrorOK) {
          client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error"); //all value set is exist
      }
      if(!value->CastSet()->contains(member)) {
        reliable = false;
        break;
      }
    }
    if(reliable == true) {
      desSet->emplace(member);
    }
  }

  if(desSet->empty()) {
    // delete key like DelCmd
    if(PSTORE.DeleteKey(client->Key())) {
      PSTORE.ClearExpire(client->Key());
    }
    client->AppendInteger(0);

  }else {
    client->AppendInteger(desSet->size());
  }

}
}  // namespace pikiwidb