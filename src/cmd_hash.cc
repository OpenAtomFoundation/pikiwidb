/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"

#include "store.h"

namespace pikiwidb {

HSetCmd::HSetCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryHash) {}

bool HSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  return true;
}

void HSetCmd::DoCmd(PClient* client) {
  int32_t ret = 0;
  storage::Status s;
  auto fvs = client->Fvs();

  for (size_t i = 2; i < client->argv_.size(); i += 2) {
    auto field = client->argv_[i];
    auto value = client->argv_[i + 1];
    fvs.push_back({field, value});
    int32_t temp = 0;
    // TODO(century): current bw doesn't support multiple fvs, fix it when necessary
    s = PSTORE.GetBackend()->HSet(client->Key(), field, value, &temp);
    if (s.ok()) {
      ret += temp;
    } else {
      // FIXME(century): need txn, if bw crashes, it should rollback
      client->SetRes(CmdRes::kErrOther);
      client->SetStatus(s);
      return;
    }
  }

  client->SetValue(PObject::CreateHash(&client->Fvs()));
  client->AppendInteger(ret);
  client->SetStatus(s);
}

void HSetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HSetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
    if (err != kPErrorOK && err != kPErrorNotExist) {
      // error occurs, do nothing
      return;
    }

    if (err == kPErrorNotExist) {
      PObject object(kPTypeHash);
      object.Reset(new PHash);
      obj = PSTORE.SetValue(client->Key(), std::move(object));
    }

    // only if key exists will update cache
    auto hash = obj->CastHash();
    // update or insert cache in fvs
    for (const auto& fv : client->Fvs()) {
      hash->insert_or_assign(fv.field, fv.value);
    }
  }
}

HGetCmd::HGetCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  return true;
}

void HGetCmd::DoCmd(PClient* client) {
  PString value;
  auto field = client->argv_[2];
  storage::Status s = PSTORE.GetBackend()->HGet(client->Key(), field, &value);
  if (s.ok()) {
    client->AppendString(value);
    client->Fvs().push_back({field, value});
  } else if (s.IsNotFound()) {
    client->SetStatus(s);
    client->AppendString("");
    return;
  } else {
    client->SetStatus(s);
    client->SetRes(CmdRes::kSyntaxErr, "hget cmd error");
    return;
  }

  client->SetValue(PObject::CreateHash(&client->Fvs()));
  client->SetStatus(s);
}

void HGetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HGetCmd::ReadCache(PClient* client) {
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
  // key not exist
  if (err != kPErrorOK) {
    client->SetRes(CmdRes::kCacheMiss);
    return;
  }

  auto hash = obj->CastHash();
  auto it = hash->find(client->argv_[2]);
  if (it != hash->end()) {
    client->AppendString(it->second);
  } else {
    // key-field not exist
    client->SetRes(CmdRes::kCacheMiss);
  }
}

void HGetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
    // key not exist, create one
    if (err == kPErrorNotExist) {
      PObject object(kPTypeHash);
      object.Reset(new PHash);
      obj = PSTORE.SetValue(client->Key(), std::move(object));
    }

    for (const auto& fv : client->Fvs()) {
      auto hash = obj->CastHash();
      hash->insert_or_assign(fv.field, fv.value);
    }
  }
}

HMSetCmd::HMSetCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryHash) {}

bool HMSetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  // set fvs
  for (size_t index = 2; index < client->argv_.size(); index += 2) {
    client->Fvs().push_back({client->argv_[index], client->argv_[index + 1]});
  }
  return true;
}

void HMSetCmd::DoCmd(PClient* client) {
  storage::Status s = PSTORE.GetBackend()->HMSet(client->Key(), client->Fvs());
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    client->SetStatus(s);
    return;
  }
  client->SetValue(PObject::CreateHash(&client->Fvs()));
  client->SetStatus(s);
}

void HMSetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HMSetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
    if (err == kPErrorNotExist) {
      PObject object(kPTypeHash);
      object.Reset(new PHash);
      obj = PSTORE.SetValue(client->Key(), std::move(object));
    }
    auto hash = obj->CastHash();
    // update or insert cache in fvs
    for (auto& fv : client->Fvs()) {
      hash->insert_or_assign(fv.field, fv.value);
    }
  }
}

HMGetCmd::HMGetCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HMGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFields();
  client->ClearFvs();
  for (size_t i = 2; i < client->argv_.size(); ++i) {
    client->Fields().push_back(client->argv_[i]);
  }
  return true;
}

void HMGetCmd::DoCmd(PClient* client) {
  std::vector<storage::ValueStatus> vss;
  auto s = PSTORE.GetBackend()->HMGet(client->Key(), client->Fields(), &vss);
  if (s.ok() || s.IsNotFound()) {
    client->AppendArrayLenUint64(vss.size());
    for (size_t i = 0; i < vss.size(); ++i) {
      if (vss[i].status.ok()) {
        client->AppendStringLenUint64(vss[i].value.size());
        client->AppendContent(vss[i].value);
        // update fvs
        client->Fvs().push_back({client->Fields()[i], vss[i].value});
      } else {
        client->AppendContent("$-1");
        client->SetStatus(s);
      }
    }
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void HMGetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HMGetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
    if (err == kPErrorNotExist) {
      PObject object(kPTypeHash);
      object.Reset(new PHash);
      obj = PSTORE.SetValue(client->Key(), std::move(object));
    }

    auto hash = obj->CastHash();
    for (auto& fv : client->Fvs()) {
      hash->insert_or_assign(fv.field, fv.value);
    }
  }
}

void HMGetCmd::ReadCache(PClient* client) {
  std::vector<storage::ValueStatus> vss;
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeHash);
  // key not exists, cache miss
  if (err != kPErrorOK) {
    client->SetRes(CmdRes::kCacheMiss);
    return;
  }

  bool cacheMiss = false;
  auto hash = obj->CastHash();
  for (const auto& field : client->Fields()) {
    if (!hash->contains(field)) {
      cacheMiss = true;
      break;
    }
  }

  if (cacheMiss) {
    client->SetRes(CmdRes::kCacheMiss);
    return;
  }

  // cache hit, return corresponding value
  for (const auto& field : client->Fields()) {
    const auto& value = hash->at(field);
    client->AppendStringLenUint64(value.size());
    client->AppendString(value);
  }
}

HGetAllCmd::HGetAllCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HGetAllCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  return true;
}

void HGetAllCmd::DoCmd(PClient* client) {
  int64_t total_fv = 0;
  int64_t cursor = 0;
  int64_t next_cursor = 0;
  size_t raw_limit = g_config.max_client_response_size;
  std::string raw;
  std::vector<storage::FieldValue> fvs;
  storage::Status s;

  do {
    fvs.clear();
    s = PSTORE.GetBackend()->HScan(client->Key(), cursor, "*", PIKA_SCAN_STEP_LENGTH, &fvs, &next_cursor);
    if (!s.ok()) {
      raw.clear();
      total_fv = 0;
      if (s.IsNotFound()) {
        client->AppendArrayLen(0);
      } else {
        client->SetRes(CmdRes::kErrOther, s.ToString());
      }
      break;
    } else {
      client->AppendArrayLenUint64(fvs.size() * 2);
      for (const auto& fv : fvs) {
        client->AppendStringLenUint64(fv.field.size());
        client->AppendContent(fv.field);
        client->AppendStringLenUint64(fv.value.size());
        client->AppendContent(fv.value);
      }
      if (raw.size() >= raw_limit) {
        client->SetRes(CmdRes::kErrOther, "Response exceeds the max-client-response-size limit");
        return;
      }
      total_fv += static_cast<int64_t>(fvs.size());
      cursor = next_cursor;
    }
  } while (cursor != 0);

  client->SetStatus(s);
}

void HGetAllCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HGetAllCmd::DoUpdateCache(PClient* client) {
  // TODO(century): find a proper way to update cache in HGetAll
}

void HGetAllCmd::ReadCache(PClient* client) {
  // TODO(century): find a proper way to read cache in HGetAll
  client->SetRes(CmdRes::kCacheMiss);
}

HKeysCmd::HKeysCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HKeysCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFields();
  return true;
}

void HKeysCmd::DoCmd(PClient* client) {
  std::vector<std::string> fields;
  auto s = PSTORE.GetBackend()->HKeys(client->Key(), &fields);
  if (s.ok() || s.IsNotFound()) {
    client->AppendArrayLenUint64(fields.size());
    for (const auto& field : fields) {
      client->AppendStringLenUint64(field.size());
      client->AppendContent(field);
    }
    // update fields
    client->Fields() = std::move(fields);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void HKeysCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HKeysCmd::ReadCache(PClient* client) {
  // TODO(century): find a proper way to read cache in HKeys
  client->SetRes(CmdRes::kCacheMiss);
}

void HKeysCmd::DoUpdateCache(PClient* client) {
  // TODO(century): find a proper way to update cache in HKeys
}

HLenCmd::HLenCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HLenCmd::DoCmd(PClient* client) {
  int32_t len = 0;
  auto s = PSTORE.GetBackend()->HLen(client->Key(), &len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(len);
  } else {
    client->SetRes(CmdRes::kErrOther, "something wrong in hlen");
  }
}

void HLenCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HLenCmd::ReadCache(PClient* client) {
  // TODO(century): find a proper way to read cache in HLen
  client->SetRes(CmdRes::kCacheMiss);
}

void HLenCmd::DoUpdateCache(PClient* client) {
  // TODO(century): find a proper way to update cache in HLen
}

HStrLenCmd::HStrLenCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryHash) {}

bool HStrLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HStrLenCmd::DoCmd(PClient* client) {
  int32_t len = 0;
  auto s = PSTORE.GetBackend()->HStrlen(client->Key(), client->argv_[2], &len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(len);
  } else {
    client->SetRes(CmdRes::kErrOther, "something wrong in hstrlen");
  }
}

void HStrLenCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void HStrLenCmd::ReadCache(PClient* client) {
  // TODO(century): find a proper way to read cache in HLen
  client->SetRes(CmdRes::kCacheMiss);
}

void HStrLenCmd::DoUpdateCache(PClient* client) {
  // TODO(century): find a proper way to update cache in HLen
}

}  // namespace pikiwidb
