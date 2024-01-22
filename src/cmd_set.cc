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
  int32_t reply_Num = 0;  // only change to 1 if ismember . key not exist it is 0
  PSTORE.GetBackend()->SIsmember(client->Key(), client->argv_[2], &reply_Num);

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
  std::vector<std::string> keys(client->Keys().begin() + 1, client->Keys().end());
  std::vector<std::string> value_to_dest;
  int32_t ret = 0;
  storage::Status s = PSTORE.GetBackend()->SUnionstore(client->Keys().at(0), keys, value_to_dest, &ret);
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
  storage::Status s = PSTORE.GetBackend()->SInter(client->Keys(), &res_vt);
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
  storage::Status s = PSTORE.GetBackend()->SRem(client->Key(), to_delete_members, &reply_num);
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
  storage::Status s = PSTORE.GetBackend()->SUnion(client->Keys(), &res_vt);
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

  std::vector<std::string> value_to_dest;int32_t reply_num = 0;

  std::vector<std::string> inter_keys(client->argv_.begin()+2,client->argv_.end());
  storage::Status s = PSTORE.GetBackend()->SInterstore(client->Key(),inter_keys,value_to_dest,&reply_num);
  if (!s.ok()) {
    client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
    return ;
  }
  client->AppendInteger(reply_num);

  //todo delete if test success
  return ;
  PObject* value = nullptr;
  PError err{};
  // check all value set is exist or not
  bool reliable = true;
  for (int i = 2; i < client->argv_.size(); ++i) {
    err = PSTORE.GetValueByType(client->argv_[i], value, kPTypeSet);
    if (err != kPErrorOK) {
      if (err != kPErrorNotExist) {
        client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
      }
      reliable = false;
      break;
    }
  }
  if (!reliable) {
    // some value set is not exist, so return
    err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
    if (err != kPErrorOK && err != kPErrorNotExist) {
      client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");
    } else {
      // delete key like DelCmd
      if (PSTORE.DeleteKey(client->Key())) {
        PSTORE.ClearExpire(client->Key());
      }
      client->AppendInteger(0);
    }
    return;
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
  err = PSTORE.GetValueByType(client->argv_[2], value, kPTypeSet);  // get girst value key

  PSET firstSet = value->CastSet();
  for (const auto& member : *firstSet) {
    reliable = true;  // here reliable used to check for a string, whether all value set contains
    for (int i = 3; i < client->argv_.size(); ++i) {  // start from second value key
      err = PSTORE.GetValueByType(client->argv_[i], value, kPTypeSet);
      if (err != kPErrorOK) {
        client->SetRes(CmdRes::kSyntaxErr, "sinterstore cmd error");  // all value set is exist
      }
      if (!value->CastSet()->contains(member)) {
        reliable = false;
        break;
      }
    }
    if (reliable == true) {
      desSet->emplace(member);
    }
  }

  if (desSet->empty()) {
    // delete key like DelCmd
    if (PSTORE.DeleteKey(client->Key())) {
      PSTORE.ClearExpire(client->Key());
    }
    client->AppendInteger(0);

  } else {
    client->AppendInteger(desSet->size());
  }
}
}  // namespace pikiwidb
