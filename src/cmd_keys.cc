/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_keys.h"

#include "pstd_string.h"

#include "store.h"

namespace pikiwidb {

DelCmd::DelCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool DelCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void DelCmd::DoCmd(PClient* client) {
  int64_t count = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Del(client->Keys());
  if (count >= 0) {
    client->AppendInteger(count);
  } else {
    client->SetRes(CmdRes::kErrOther, "delete error");
  }
}

ExistsCmd::ExistsCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryKeyspace) {}

bool ExistsCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin() + 1, client->argv_.end());
  client->SetKey(keys);
  return true;
}

void ExistsCmd::DoCmd(PClient* client) {
  int64_t count = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Exists(client->Keys());
  if (count >= 0) {
    client->AppendInteger(count);
    //    if (PSTORE.ExistsKey(client->Key())) {
    //      client->AppendInteger(1);
    //    } else {
    //      client->SetRes(CmdRes::kErrOther, "exists internal error");
    //    }
  } else {
    client->SetRes(CmdRes::kErrOther, "exists internal error");
  }
}

PExpireCmd::PExpireCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool PExpireCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void PExpireCmd::DoCmd(PClient* client) {
  int64_t msec = 0;
  if (pstd::String2int(client->argv_[2], &msec) == 0) {
    client->SetRes(CmdRes ::kInvalidInt);
    return;
  }
  auto res = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Expire(client->Key(), msec / 1000);
  if (res != -1) {
    client->AppendInteger(res);
  } else {
    client->SetRes(CmdRes::kErrOther, "pexpire internal error");
  }
}

ExpireatCmd::ExpireatCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool ExpireatCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ExpireatCmd::DoCmd(PClient* client) {
  int64_t time_stamp = 0;
  if (pstd::String2int(client->argv_[2], &time_stamp) == 0) {
    client->SetRes(CmdRes ::kInvalidInt);
    return;
  }
  auto res = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Expireat(client->Key(), time_stamp);
  if (res != -1) {
    client->AppendInteger(res);
  } else {
    client->SetRes(CmdRes::kErrOther, "expireat internal error");
  }
}

PExpireatCmd::PExpireatCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool PExpireatCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

// PExpireatCmd actually invoke Expireat
void PExpireatCmd::DoCmd(PClient* client) {
  int64_t time_stamp_ms = 0;
  if (pstd::String2int(client->argv_[2], &time_stamp_ms) == 0) {
    client->SetRes(CmdRes ::kInvalidInt);
    return;
  }
  auto res = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Expireat(client->Key(), time_stamp_ms / 1000);
  if (res != -1) {
    client->AppendInteger(res);
  } else {
    client->SetRes(CmdRes::kErrOther, "pexpireat internal error");
  }
}

PersistCmd::PersistCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryKeyspace) {}

bool PersistCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void PersistCmd::DoCmd(PClient* client) {
  std::map<storage::DataType, storage::Status> type_status;
  auto res = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Persist(client->Key(), &type_status);
  if (res != -1) {
    client->AppendInteger(res);
  } else {
    std::string cnt;
    for (auto const& s : type_status) {
      cnt.append(storage::DataTypeToString[s.first]);
      cnt.append(" - ");
      cnt.append(s.second.ToString());
      cnt.append(";");
    }
    client->SetRes(CmdRes::kErrOther, cnt);
  }
}

KeysCmd::KeysCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryKeyspace) {}

bool KeysCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void KeysCmd::DoCmd(PClient* client) {
  std::vector<std::string> keys;
  auto s = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->Keys(storage::DataType::kAll, client->Key(), &keys);
  if (s.ok()) {
    client->AppendArrayLen(keys.size());
    for (auto k : keys) {
      client->AppendString(k);
    }
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

PttlCmd::PttlCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryKeyspace) {}

bool PttlCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

// like Blackwidow , Floyd still possible has same key in different data structure
void PttlCmd::DoCmd(PClient* client) {
  std::map<storage::DataType, rocksdb::Status> type_status;
  auto type_timestamp = PSTORE.GetBackend(client->GetCurrentDB())->GetStorage()->TTL(client->Key(), &type_status);
  for (const auto& item : type_timestamp) {
    // mean operation exception errors happen in database
    if (item.second == -3) {
      client->SetRes(CmdRes::kErrOther, "ttl internal error");
      return;
    }
  }
  if (type_timestamp[storage::kStrings] != -2) {
    if (type_timestamp[storage::kStrings] == -1) {
      client->AppendInteger(-1);
    } else {
      client->AppendInteger(type_timestamp[storage::kStrings] * 1000);
    }
  } else if (type_timestamp[storage::kHashes] != -2) {
    if (type_timestamp[storage::kHashes] == -1) {
      client->AppendInteger(-1);
    } else {
      client->AppendInteger(type_timestamp[storage::kHashes] * 1000);
    }
  } else if (type_timestamp[storage::kLists] != -2) {
    if (type_timestamp[storage::kLists] == -1) {
      client->AppendInteger(-1);
    } else {
      client->AppendInteger(type_timestamp[storage::kLists] * 1000);
    }
  } else if (type_timestamp[storage::kSets] != -2) {
    if (type_timestamp[storage::kSets] == -1) {
      client->AppendInteger(-1);
    } else {
      client->AppendInteger(type_timestamp[storage::kSets] * 1000);
    }
  } else if (type_timestamp[storage::kZSets] != -2) {
    if (type_timestamp[storage::kZSets] == -1) {
      client->AppendInteger(-1);
    } else {
      client->AppendInteger(type_timestamp[storage::kZSets] * 1000);
    }
  } else {
    // this key not exist
    client->AppendInteger(-2);
  }
}

}  // namespace pikiwidb
