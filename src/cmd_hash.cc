/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"

#include "pstd/pstd_string.h"
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK && err != kPErrorNotExist) {
    ReplyError(err, &reply);
    client->SetRes(CmdRes::kSyntaxErr, "hset cmd error");
    return;
  }
  if (err == kPErrorNotExist) {
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK && err != kPErrorNotExist) {
    ReplyError(err, &reply);
    client->SetRes(CmdRes::kSyntaxErr, "hmset cmd error");
    return;
  }
  if (err == kPErrorNotExist) {
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
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
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
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

HLenCmd::HLenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HLenCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hlen cmd error");
    }
    return;
  }

  auto hash = value->CastHash();
  FormatInt(hash->size(), &reply);
  client->AppendStringRaw(reply.ReadAddr());
}

HStrLenCmd::HStrLenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HStrLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HStrLenCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  UnboundedBuffer reply;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, &reply);
    if (err == kPErrorNotExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hstrlen cmd error");
    }
    return;
  }

  auto hash = value->CastHash();
  auto it = hash->find(client->argv_[2]);
  if (it == hash->end()) {
    Format0(&reply);
  } else {
    FormatInt(it->second.size(), &reply);
  }

  client->AppendStringRaw(reply.ReadAddr());
}

HRandFieldCmd::HRandFieldCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HRandFieldCmd::DoInitial(PClient* client) {
  /*
   * There should not be quantity detection here,
   * because the quantity detection of redis is after the COUNT integer detection.
   */
  // if (client->argv_.size() > 4) {
  //   client->SetRes(CmdRes::kSyntaxErr);
  //   return false;
  // }
  client->SetKey(client->argv_[1]);
  return true;
}

void HRandFieldCmd::DoCmd(PClient* client) {
  // parse arguments
  int64_t count{};
  bool with_values{false};
  if (client->argv_.size() > 2) {
    if (pstd::String2int(client->argv_[2], &count) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }

    if (client->argv_.size() > 4) {
      client->SetRes(CmdRes::kSyntaxErr);
      return;
    }

    if (client->argv_.size() > 3) {
      if (kWithValueString != pstd::StringToLower(client->argv_[3])) {
        client->SetRes(CmdRes::kSyntaxErr);
        return;
      }
      with_values = true;
    }
  }

  // get hash
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeHash);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "hrandfield cmd error");
    }
    return;
  }
  auto hash = value->CastHash();
  if (hash->empty()) {
    client->AppendString("");
    return;
  }

  // fetch field(s) and reply
  if (client->argv_.size() > 2) {
    if (count >= 0) {
      DoWithPositiveCount(client, hash, count, with_values);
    } else {
      DoWithNegativeCount(client, hash, count, with_values);
    }
  } else {
    auto it = std::next(hash->begin(), rand() % hash->size());
    client->AppendString(it->first);
  }
}

void HRandFieldCmd::DoWithPositiveCount(PClient* client, const PHash* hash, int64_t count, bool with_value) {
  if (hash->size() <= count) {  // reply all fields
    client->AppendArrayLen(with_value ? hash->size() * 2 : hash->size());
    for (auto&& kv : *hash) {
      client->AppendString(kv.first);
      if (with_value) {
        client->AppendString(kv.second);
      }
    }
  } else {  // reply [count] fields
    std::vector<std::pair<PString, PString>> kvs;
    for (auto&& kv : *hash) {
      kvs.push_back(kv);
    }
    std::shuffle(kvs.begin(), kvs.end(), std::mt19937(rd_()));

    client->AppendArrayLen(with_value ? count * 2 : count);
    for (size_t i = 0; i < count; i++) {
      client->AppendString(kvs[i].first);
      if (with_value) {
        client->AppendString(kvs[i].second);
      }
    }
  }
}

void HRandFieldCmd::DoWithNegativeCount(PClient* client, const PHash* hash, int64_t count, bool with_value) {
  count = -count;
  client->AppendArrayLen(with_value ? count * 2 : count);
  while (count--) {
    auto it = std::next(hash->begin(), rand() % hash->size());
    client->AppendString(it->first);
    if (with_value) {
      client->AppendString(it->second);
    }
  }
}

}  // namespace pikiwidb
