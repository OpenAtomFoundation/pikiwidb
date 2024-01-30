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

<<<<<<< HEAD
<<<<<<< HEAD
SUnionCmd::SUnionCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SUnionCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
=======
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
>>>>>>> 903857d (feat:add set cmd sadd)
  client->SetKey(keys);
  return true;
}

<<<<<<< HEAD
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
=======
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
>>>>>>> 903857d (feat:add set cmd sadd)
  client->SetKey(client->argv_[1]);
  return true;
}

<<<<<<< HEAD
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
}  // namespace pikiwidb
=======
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

<<<<<<< HEAD
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
=======
SCardCmd::SCardCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategorySet) {}

bool SCardCmd::DoInitial(PClient* client) {
>>>>>>> 3abd6be (feat:add set cmd scard)
  client->SetKey(client->argv_[1]);
  return true;
}

<<<<<<< HEAD
void SRemCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
  int retVal = 0;
=======
void SCardCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeSet);
>>>>>>> 3abd6be (feat:add set cmd scard)
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      client->AppendInteger(0);
    } else {
<<<<<<< HEAD
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
}  // namespace pikiwidb
>>>>>>> 903857d (feat:add set cmd sadd)
=======
}  // namespace pikiwidb
>>>>>>> 15c4a11 (resolve conflict)
=======
      client->SetRes(CmdRes::kErrOther);
    }
    return;
  }
  client->AppendInteger(value->CastSet()->size());
}

}  // namespace pikiwidb
>>>>>>> 3abd6be (feat:add set cmd scard)
