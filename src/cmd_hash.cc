/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"

#include "store.h"

namespace pikiwidb {

HSetCmd::HSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryHash) {}

bool HSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void HSetCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok && err != PError_notExist) {
    ReplyError(err, &reply);
    client->SetRes(CmdRes::kSyntaxErr, "hset cmd error");
    return;
  }
  if (err == PError_notExist) {
    value = PSTORE.SetValue(client->Key(), PObject::CreateHash());
  }

  auto new_cnt = 0;
  auto hash = value->CastHash();
  for (size_t i = 2; i < client->argv_.size(); i += 2) {
    auto field = client->argv_[i];
    auto value = client->argv_[i + 1];
    auto it = hash->find(field);
    if (it == hash->end()) {
      hash->insert(PHash::value_type(field, value));
      ++new_cnt;
    } else {
      it->second = value;
    }
  }
  FormatInt(new_cnt, &reply);
  client->AppendStringRaw(reply.ReadAddr());
}

HGetCmd::HGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok) {
    ReplyError(err, &reply);
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hget cmd error");
    }
    return;
  }

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
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryHash) {}

bool HMSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHMSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void HMSetCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok && err != PError_notExist) {
    ReplyError(err, &reply);
    client->SetRes(CmdRes::kSyntaxErr, "hmset cmd error");
    return;
  }
  if (err == PError_notExist) {
    value = PSTORE.SetValue(client->Key(), PObject::CreateHash());
  }

  auto hash = value->CastHash();
  for (size_t i = 2; i < client->argv_.size(); i += 2) {
    auto field = client->argv_[i];
    auto value = client->argv_[i + 1];
    auto it = hash->find(field);
    if (it == hash->end()) {
      hash->insert(PHash::value_type(field, value));
    } else {
      it->second = value;
    }
  }
  FormatOK(&reply);
  client->AppendStringRaw(reply.ReadAddr());
}

HMGetCmd::HMGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HMGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HMGetCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok) {
    ReplyError(err, &reply);
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hmget cmd error");
    }
    return;
  }

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
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HGetAllCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetAllCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok) {
    ReplyError(err, &reply);
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hgetall cmd error");
    }
    return;
  }

  auto hash = value->CastHash();
  PreFormatMultiBulk(2 * hash->size(), &reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, &reply);
    FormatBulk(kv.second, &reply);
  }
  client->AppendStringRaw(reply.ReadAddr());
}

HKeysCmd::HKeysCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HKeysCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HKeysCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok) {
    ReplyError(err, &reply);
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hkeys cmd error");
    }
    return;
  }

  auto hash = value->CastHash();
  PreFormatMultiBulk(hash->size(), &reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, &reply);
  }
  client->AppendStringRaw(reply.ReadAddr());
}

}  // namespace pikiwidb
