/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_kv.h"
#include "common.h"
#include "pstd_string.h"
#include "pstd_util.h"
#include "store.h"

namespace pikiwidb {

GetCmd::GetCmd(const std::string& name, const int16_t arity, const uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool GetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetCmd::DoCmd(PClient* client) {
  PString value;
  int64_t ttl = -1;
  storage::Status s = PSTORE.GetBackend()->GetWithTTL(client->Key(), &value, &ttl);
  if (s.ok()) {
    client->AppendString(value);
    client->SetValue(PObject::CreateString(value));
  } else if (s.IsNotFound()) {
    client->AppendString("");
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "get key error");
  }
  client->SetStatus(s);
}

void GetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void GetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

void GetCmd::ReadCache(PClient* client) {
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeString);
  if (err == kPErrorOK) {
    auto value = obj->CastString();
    client->AppendString(*value);
  } else {
    client->SetRes(CmdRes::kCacheMiss);
  }
}

SetCmd::SetCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool SetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetCmd::DoCmd(PClient* client) {
  storage::Status s = PSTORE.GetBackend()->Set(client->Key(), client->argv_[2]);
  if (s.ok()) {
    client->SetValue(PObject::CreateString(client->argv_[2]));
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void SetCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void SetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

BitOpCmd::BitOpCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool BitOpCmd::DoInitial(PClient* client) {
  if (!(pstd::StringEqualCaseInsensitive(client->argv_[1], "and") ||
        pstd::StringEqualCaseInsensitive(client->argv_[1], "or") ||
        pstd::StringEqualCaseInsensitive(client->argv_[1], "not") ||
        pstd::StringEqualCaseInsensitive(client->argv_[1], "xor"))) {
    client->SetRes(CmdRes::kSyntaxErr, "operation error");
    return false;
  }
  return true;
}

void BitOpCmd::DoCmd(PClient* client) {
  std::vector<std::string> keys;
  for (size_t i = 3; i < client->argv_.size(); ++i) {
    keys.push_back(client->argv_[i]);
  }

  PError err = kPErrorParam;
  PString res;
  storage::BitOpType op = storage::kBitOpDefault;

  if (keys.size() == 1) {
    if (pstd::StringEqualCaseInsensitive(client->argv_[1], "or")) {
      err = kPErrorOK;
      op = storage::kBitOpOr;
    }
  } else if (keys.size() >= 2) {
    if (pstd::StringEqualCaseInsensitive(client->argv_[1], "xor")) {
      err = kPErrorOK;
      op = storage::kBitOpXor;
    } else if (pstd::StringEqualCaseInsensitive(client->argv_[1], "and")) {
      err = kPErrorOK;
      op = storage::kBitOpAnd;
    } else if (pstd::StringEqualCaseInsensitive(client->argv_[1], "not")) {
      if (client->argv_.size() == 4) {
        err = kPErrorOK;
        op = storage::kBitOpNot;
      }
    }
  }

  if (err != kPErrorOK) {
    client->SetRes(CmdRes::kSyntaxErr);
  } else {
    PString value;
    int64_t result_length = 0;
    storage::Status s = PSTORE.GetBackend()->BitOp(op, client->argv_[2], keys, value, &result_length);
    if (s.ok()) {
      client->AppendInteger(result_length);
    } else {
      client->SetRes(CmdRes::kErrOther, s.ToString());
    }
    client->SetStatus(s);
  }
}

void BitOpCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void BitOpCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.DeleteKey(client->Key());
  }
}

StrlenCmd::StrlenCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool StrlenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void StrlenCmd::DoCmd(PClient* client) {
  int32_t len = 0;
  storage::Status s = PSTORE.GetBackend()->Strlen(client->Key(), &len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(len);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void StrlenCmd::DoThroughDB(PClient* client) {
  client->Clear();
  PString value;
  int64_t ttl = 0;
  storage::Status s = PSTORE.GetBackend()->GetWithTTL(client->Key(), &value, &ttl);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(value.size());
    client->SetValue(PObject::CreateString(value));
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

void StrlenCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

void StrlenCmd::ReadCache(PClient* client) {
  int32_t len = 0;
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeString);
  if (err == kPErrorOK) {
    auto value = obj->CastString();
    client->AppendInteger(value->size());
  } else {
    client->SetRes(CmdRes::kCacheMiss);
  }
}

SetExCmd::SetExCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool SetExCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  int64_t sec = 0;
  if (pstd::String2int(client->argv_[2], &sec) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  return true;
}

void SetExCmd::DoCmd(PClient* client) {
  int64_t sec = 0;
  pstd::String2int(client->argv_[2], &sec);
  storage::Status s = PSTORE.GetBackend()->Setex(client->Key(), client->argv_[3], sec);
  if (s.ok()) {
    client->SetValue(PObject::CreateString(client->argv_[3]));
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void SetExCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void SetExCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
    int64_t sec = 0;
    pstd::String2int(client->argv_[2], &sec);
    PSTORE.SetExpire(client->Key(), pstd::UnixMilliTimestamp() + sec * 1000);
  }
}

PSetExCmd::PSetExCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool PSetExCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  int64_t msec = 0;
  if (pstd::String2int(client->argv_[2], &msec) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  return true;
}

void PSetExCmd::DoCmd(PClient* client) {
  int64_t msec = 0;
  pstd::String2int(client->argv_[2], &msec);
  storage::Status s = PSTORE.GetBackend()->Setex(client->Key(), client->argv_[3], static_cast<int32_t>(msec / 1000));
  if (s.ok()) {
    client->SetValue(PObject::CreateString(client->argv_[3]));
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void PSetExCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void PSetExCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
    int64_t msec = 0;
    pstd::String2int(client->argv_[2], &msec);
    PSTORE.SetExpire(client->Key(), pstd::UnixMilliTimestamp() + msec);
  }
}

SetNXCmd::SetNXCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool SetNXCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetNXCmd::DoCmd(PClient* client) {
  int32_t success = 0;
  storage::Status s = PSTORE.GetBackend()->Setnx(client->Key(), client->argv_[2], &success);
  if (s.ok()) {
    client->SetValue(PObject::CreateString(client->argv_[2]));
    client->AppendInteger(success);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

AppendCmd::AppendCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool AppendCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void AppendCmd::DoCmd(PClient* client) {
  int32_t new_len = 0;
  storage::Status s = PSTORE.GetBackend()->Append(client->Key(), client->argv_[2], &new_len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(new_len);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void AppendCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void AppendCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PString old_value;
    int64_t ttl = -1;
    storage::Status s = PSTORE.GetBackend()->GetWithTTL(client->Key(), &old_value, &ttl);
    PString new_value = old_value + client->argv_[2];
    PSTORE.SetValue(client->Key(), PObject::CreateString(new_value));
  }
}

GetSetCmd::GetSetCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool GetSetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetSetCmd::DoCmd(PClient* client) {
  std::string old_value;
  storage::Status s = PSTORE.GetBackend()->GetSet(client->Key(), client->argv_[2], &old_value);
  if (s.ok()) {
    if (old_value.empty()) {
      client->AppendContent("$-1");
    } else {
      client->AppendStringLen(old_value.size());
      client->AppendContent(old_value);
    }
    client->SetValue(PObject::CreateString(client->argv_[2]));
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void GetSetCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void GetSetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

MGetCmd::MGetCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool MGetCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin(), client->argv_.end());
  keys.erase(keys.begin());
  client->SetKey(keys);
  return true;
}

void MGetCmd::DoCmd(PClient* client) {
  std::vector<storage::ValueStatus> db_value_status_array;
  storage::Status s = PSTORE.GetBackend()->MGet(client->Keys(), &db_value_status_array);
  if (s.ok()) {
    client->AppendArrayLen(db_value_status_array.size());
    for (const auto& vs : db_value_status_array) {
      if (vs.status.ok()) {
        client->AppendStringLen(vs.value.size());
        client->AppendContent(vs.value);
      } else {
        client->AppendContent("$-1");
      }
    }
    client->SetDBValueStatusArray(db_value_status_array);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void MGetCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void MGetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    auto keys = client->Keys();
    auto db_value_status_array = client->GetDBValueStatusArray();
    for (size_t i = 0; i < keys.size(); i++) {
      if (db_value_status_array[i].status.ok()) {
        PSTORE.SetValue(keys[i], PObject::CreateString(std::move(db_value_status_array[i].value)));
      }
    }
  }
}

void MGetCmd::ReadCache(PClient* client) {
  std::vector<storage::ValueStatus> cache_value_status_array;
  for (const auto& key : client->Keys()) {
    auto [obj, err] = PSTORE.GetValueByType(key, kPTypeString);
    if (err == kPErrorOK) {
      auto str = GetDecodedString(obj);
      std::string reply(str->c_str(), str->size());
      client->AppendStringLen(static_cast<int64_t>(reply.size()));
      client->AppendContent(reply);
    } else {
      client->SetRes(CmdRes::kCacheMiss);
      break;
    }
  }
}

MSetCmd::MSetCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool MSetCmd::DoInitial(PClient* client) {
  size_t argcSize = client->argv_.size();
  if (argcSize % 2 == 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameMSet);
    return false;
  }
  return true;
}

void MSetCmd::DoCmd(PClient* client) {
  std::vector<storage::KeyValue> kvs;
  for (size_t index = 1; index != client->argv_.size(); index += 2) {
    kvs.push_back({client->argv_[index], client->argv_[index + 1]});
  }
  storage::Status s = PSTORE.GetBackend()->MSet(kvs);
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void MSetCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void MSetCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    for (size_t index = 1; index != client->argv_.size(); index += 2) {
      PSTORE.ClearExpire(client->argv_[index]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[index], PObject::CreateString(client->argv_[index + 1]));
    }
  }
}

BitCountCmd::BitCountCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool BitCountCmd::DoInitial(PClient* client) {
  size_t paramSize = client->argv_.size();
  if (paramSize != 2 && paramSize != 4) {
    client->SetRes(CmdRes::kSyntaxErr, kCmdNameBitCount);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void BitCountCmd::DoCmd(PClient* client) {
  storage::Status s;
  int32_t count = 0;
  if (client->argv_.size() == 2) {
    s = PSTORE.GetBackend()->BitCount(client->Key(), 0, 0, &count, false);
  } else {
    int64_t start_offset = 0;
    int64_t end_offset = 0;
    if (pstd::String2int(client->argv_[2], &start_offset) == 0 ||
        pstd::String2int(client->argv_[3], &end_offset) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }

    s = PSTORE.GetBackend()->BitCount(client->Key(), start_offset, end_offset, &count, false);
  }

  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(count);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void BitCountCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void BitCountCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PString value;
    int64_t ttl = 0;
    storage::Status s = PSTORE.GetBackend()->GetWithTTL(client->Key(), &value, &ttl);
    if (s.ok()) {
      PSTORE.SetValue(client->Key(), PObject::CreateString(value));
    }
  }
}

void BitCountCmd::ReadCache(PClient* client) {
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeString);
  if (err != kPErrorOK) {
    client->SetRes(CmdRes::kCacheMiss);
    return;
  }

  auto str = GetDecodedString(obj);
  auto value_length = static_cast<int64_t>(str->size());
  int64_t start_offset = 0;
  int64_t end_offset = 0;
  if (client->argv_.size() == 4) {
    if (pstd::String2int(client->argv_[2], &start_offset) == 0 ||
        pstd::String2int(client->argv_[3], &end_offset) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }

    if (start_offset < 0) {
      start_offset += value_length;
    }
    if (end_offset < 0) {
      end_offset += value_length;
    }
    if (start_offset < 0) {
      start_offset = 0;
    }
    if (end_offset < 0) {
      end_offset = 0;
    }
    if (end_offset >= value_length) {
      end_offset = value_length - 1;
    }
  } else {
    start_offset = 0;
    end_offset = std::max(value_length - 1, static_cast<int64_t>(0));
  }

  size_t count = 0;
  if (end_offset >= start_offset) {
    count = BitCount(reinterpret_cast<const uint8_t*>(str->data()) + start_offset, end_offset - start_offset + 1);
  }
  client->AppendInteger(static_cast<int64_t>(count));
}

IncrbyCmd::IncrbyCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool IncrbyCmd::DoInitial(PClient* client) {
  int64_t by = 0;
  if (!(pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by))) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void IncrbyCmd::DoCmd(PClient* client) {
  int64_t ret = 0;
  int64_t by = 0;
  pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by);
  storage::Status s = PSTORE.GetBackend()->Incrby(client->Key(), by, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
    client->SetValue(PObject::CreateString(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void IncrbyCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void IncrbyCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

GetBitCmd::GetBitCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool GetBitCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetBitCmd::DoCmd(PClient* client) {
  int32_t bit_val = 0;
  long offset = 0;
  if (!Strtol(client->argv_[2].c_str(), client->argv_[2].size(), &offset)) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  storage::Status s = PSTORE.GetBackend()->GetBit(client->Key(), offset, &bit_val);
  if (s.ok()) {
    client->AppendInteger(bit_val);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void GetBitCmd::DoThroughDB(PClient* client) {
  client->Clear();
  DoCmd(client);
}

void GetBitCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PString value;
    int64_t ttl = 0;
    storage::Status s = PSTORE.GetBackend()->GetWithTTL(client->Key(), &value, &ttl);
    if (s.ok()) {
      PSTORE.SetValue(client->Key(), PObject::CreateString(value));
    }
  }
}

void GetBitCmd::ReadCache(PClient* client) {
  auto [obj, err] = PSTORE.GetValueByType(client->Key(), kPTypeString);
  if (err != kPErrorOK) {
    client->SetRes(CmdRes::kCacheMiss);
    return;
  }

  long offset = 0;
  if (!Strtol(client->argv_[2].c_str(), client->argv_[2].size(), &offset)) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  auto str = GetDecodedString(obj);
  const uint8_t* buf = (const uint8_t*)str->c_str();
  size_t size = 8 * str->size();

  if (offset < 0 || offset >= static_cast<long>(size)) {
    client->AppendInteger(0);
    return;
  }

  size_t bytesOffset = offset / 8;
  size_t bitsOffset = offset % 8;
  uint8_t byte = buf[bytesOffset];
  if (byte & (0x1 << bitsOffset)) {
    client->AppendInteger(1);
  } else {
    client->AppendInteger(0);
  }
}

IncrbyFloatCmd::IncrbyFloatCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool IncrbyFloatCmd::DoInitial(PClient* client) {
  long double by_ = 0.00f;
  if (StrToLongDouble(client->argv_[2].data(), client->argv_[2].size(), &by_)) {
    client->SetRes(CmdRes::kInvalidFloat);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void IncrbyFloatCmd::DoCmd(PClient* client) {
  PString ret;
  storage::Status s = PSTORE.GetBackend()->Incrbyfloat(client->Key(), client->argv_[2], &ret);
  if (s.ok()) {
    client->AppendStringLen(ret.size());
    client->AppendContent(ret);
    client->SetValue(PObject::CreateString(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a vaild float") {
    client->SetRes(CmdRes::kInvalidFloat);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::KIncrByOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void IncrbyFloatCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void IncrbyFloatCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

SetBitCmd::SetBitCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool SetBitCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetBitCmd::DoCmd(PClient* client) {
  long offset = 0;
  long on = 0;
  if (!Strtol(client->argv_[2].c_str(), client->argv_[2].size(), &offset) ||
      !Strtol(client->argv_[3].c_str(), client->argv_[3].size(), &on)) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  if (offset < 0 || offset > kStringMaxBytes) {
    client->AppendInteger(0);
    return;
  }

  if ((on & ~1) != 0) {
    client->SetRes(CmdRes::kInvalidBitInt);
    return;
  }

  PString value;
  int32_t bit_val = 0;
  storage::Status s = PSTORE.GetBackend()->SetBit(client->Key(), offset, static_cast<int32_t>(on), &bit_val);
  if (s.ok()) {
    client->AppendInteger(static_cast<int>(bit_val));
    client->SetValue(PObject::CreateString(static_cast<long>(bit_val)));
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void SetBitCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void SetBitCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

DecrbyCmd::DecrbyCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryWrite | kAclCategoryString) {}

bool DecrbyCmd::DoInitial(PClient* client) {
  int64_t by = 0;
  if (!(pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by))) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void DecrbyCmd::DoCmd(PClient* client) {
  int64_t ret = 0;
  int64_t by = 0;
  if (pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }
  storage::Status s = PSTORE.GetBackend()->Decrby(client->Key(), by, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
    client->SetValue(PObject::CreateString(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void DecrbyCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void DecrbyCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

DecrCmd::DecrCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool DecrCmd::DoInitial(pikiwidb::PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void DecrCmd::DoCmd(pikiwidb::PClient* client) {
  int64_t ret = 0;
  storage::Status s = PSTORE.GetBackend()->Decrby(client->Key(), 1, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
    client->SetValue(PObject::CreateString(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void DecrCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void DecrCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

IncrCmd::IncrCmd(const std::string& name, int16_t arity, uint32_t flag)
    : BaseCmd(name, arity, flag, kAclCategoryRead | kAclCategoryString) {}

bool IncrCmd::DoInitial(pikiwidb::PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void IncrCmd::DoCmd(pikiwidb::PClient* client) {
  int64_t ret = 0;
  storage::Status s = PSTORE.GetBackend()->Incrby(client->Key(), 1, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
    client->SetValue(PObject::CreateString(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
  client->SetStatus(s);
}

void IncrCmd::DoThroughDB(PClient* client) { DoCmd(client); }

void IncrCmd::DoUpdateCache(PClient* client) {
  if (client->GetStatus().ok() && client->Ok()) {
    PSTORE.ClearExpire(client->Key());  // clear key's old ttl
    PSTORE.SetValue(client->Key(), std::move(client->Value()));
  }
}

}  // namespace pikiwidb
