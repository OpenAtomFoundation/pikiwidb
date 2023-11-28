/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"

#include "store.h"

namespace pikiwidb {

#define GET_HASH(cmd)                                                   \
  PObject* value;                                                       \
  UnboundedBuffer reply;                                                \
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash); \
  if (err != PError_ok) {                                               \
    ReplyError(err, &reply);                                            \
    if (err == PError_notExist) {                                       \
      client->AppendString("");                                         \
    } else {                                                            \
      client->SetRes(CmdRes::kSyntaxErr, #cmd " cmd error");            \
    }                                                                   \
    return;                                                             \
  }

#define GET_OR_SET_HASH(cmd)                                            \
  PObject* value;                                                       \
  UnboundedBuffer reply;                                                \
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash); \
  if (err != PError_ok && err != PError_notExist) {                     \
    ReplyError(err, &reply);                                            \
    client->SetRes(CmdRes::kSyntaxErr, #cmd " cmd error");              \
    return;                                                             \
  }                                                                     \
  if (err == PError_notExist) {                                         \
    value = PSTORE.SetValue(client->Key(), PObject::CreateHash());      \
  }

static inline PHash::iterator _set_hash_force(PHash& hash, const PString& key, const PString& val) {
  auto it(hash.find(key));
  if (it != hash.end()) {
    it->second = val;
  } else {
    it = hash.insert(PHash::value_type(key, val)).first;
  }
  return it;
}

HGetCmd::HGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetCmd::DoCmd(PClient* client) {
  GET_HASH(hget);

  auto hash = value->CastHash();
  auto it = hash->find(client->argv_[2]);

  if (it != hash->end()) {
    FormatBulk(it->second, &reply);
  } else {
    FormatNull(&reply);
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HMSetCmd::HMSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryHash) {}

bool HMSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHMSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void HMSetCmd::DoCmd(PClient* client) {
  GET_OR_SET_HASH(hmset);

  auto hash = value->CastHash();
  for (size_t i = 2; i < client->argv_.size(); i += 2) {
    _set_hash_force(*hash, client->argv_[i], client->argv_[i + 1]);
  }
  FormatOK(&reply);
  client->AppendStringRaw(reply.ReadAddr());
}

HMGetCmd::HMGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HMGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HMGetCmd::DoCmd(PClient* client) {
  GET_HASH(hmget);

  auto hash = value->CastHash();
  PreFormatMultiBulk(client->argv_.size() - 2, &reply);

  for (size_t i = 2; i < client->argv_.size(); ++i) {
    auto it = hash->find(client->argv_[i]);
    if (it != hash->end()) {
      FormatBulk(it->second, &reply);
    } else {
      FormatNull(&reply);
    }
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HGetAllCmd::HGetAllCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HGetAllCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetAllCmd::DoCmd(PClient* client) {
  GET_HASH(hgetall);

  auto hash = value->CastHash();
  PreFormatMultiBulk(2 * hash->size(), &reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, &reply);
    FormatBulk(kv.second, &reply);
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HKeysCmd::HKeysCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryHash) {}

bool HKeysCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HKeysCmd::DoCmd(PClient* client) {
  GET_HASH(hkeys);

  auto hash = value->CastHash();
  PreFormatMultiBulk(hash->size(), &reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, &reply);
  }
  client->AppendStringRaw(reply.ReadAddr());
}

}  // namespace pikiwidb
