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

GetCmd::GetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

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
  } else if (s.IsNotFound()) {
    client->AppendString("");
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "get key error");
  }
}

SetCmd::SetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool SetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetCmd::DoCmd(PClient* client) {
  storage::Status s = PSTORE.GetBackend()->Set(client->Key(), client->argv_[2]);
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

AppendCmd::AppendCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
}

GetSetCmd::GetSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

MGetCmd::MGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

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
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

MSetCmd::MSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool MSetCmd::DoInitial(PClient* client) {
  size_t argcSize = client->argv_.size();
  if (argcSize % 2 == 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameMSet);
    return false;
  }
  std::vector<std::string> keys;
  for (size_t index = 1; index < argcSize; index += 2) {
    keys.emplace_back(client->argv_[index]);
  }
  client->SetKey(keys);
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
}

BitCountCmd::BitCountCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

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
}

DecrCmd::DecrCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

bool DecrCmd::DoInitial(pikiwidb::PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void DecrCmd::DoCmd(pikiwidb::PClient* client) {
  int64_t ret = 0;
  storage::Status s = PSTORE.GetBackend()->Decrby(client->Key(), 1, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

IncrCmd::IncrCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

bool IncrCmd::DoInitial(pikiwidb::PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void IncrCmd::DoCmd(pikiwidb::PClient* client) {
  int64_t ret = 0;
  storage::Status s = PSTORE.GetBackend()->Incrby(client->Key(), 1, &ret);
  if (s.ok()) {
    client->AppendContent(":" + std::to_string(ret));
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

BitOpCmd::BitOpCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
  }
}

StrlenCmd::StrlenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

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
}

SetExCmd::SetExCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

PSetExCmd::PSetExCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

IncrbyCmd::IncrbyCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool IncrbyCmd::DoInitial(PClient* client) {
  int64_t by_ = 0;
  if (!(pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by_))) {
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
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  };
}

DecrbyCmd::DecrbyCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a integer") {
    client->SetRes(CmdRes::kInvalidInt);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::kOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

IncrbyFloatCmd::IncrbyFloatCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
  } else if (s.IsCorruption() && s.ToString() == "Corruption: Value is not a vaild float") {
    client->SetRes(CmdRes::kInvalidFloat);
  } else if (s.IsInvalidArgument()) {
    client->SetRes(CmdRes::KIncrByOverFlow);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

SetNXCmd::SetNXCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

bool SetNXCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetNXCmd::DoCmd(PClient* client) {
  int32_t success = 0;
  storage::Status s = PSTORE.GetBackend()->Setnx(client->Key(), client->argv_[2], &success);
  if (s.ok()) {
    client->AppendInteger(success);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

GetBitCmd::GetBitCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
}

GetRangeCmd::GetRangeCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryString) {}

bool GetRangeCmd::DoInitial(PClient* client) {
  // > range key start end
  int64_t start = 0;
  int64_t end = 0;
  // ERR value is not an integer or out of range
  if (!(pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &start)) ||
      !(pstd::String2int(client->argv_[3].data(), client->argv_[3].size(), &end))) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void GetRangeCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, kPTypeString);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kErrOther, "getrange cmd error");
    }
    return;
  }

  int64_t start = 0;
  int64_t end = 0;
  pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &start);
  pstd::String2int(client->argv_[3].data(), client->argv_[3].size(), &end);

  auto str = GetDecodedString(value);
  size_t len = str->size();

  // if the start offset is greater than the end offset, return an empty string
  if (end < start) {
    client->AppendString("");
    return;
  }

  // calculate the offset
  // if it is a negative number, start from the end
  if (start < 0) {
    start += len;
  }
  if (end < 0) {
    end += len;
  }
  if (start < 0) {
    start = 0;
  }
  if (end < 0) {
    end = 0;
  }
  // if the offset exceeds the length of the string, set it to the end of the string.
  if (end >= len) {
    end = len - 1;
  }

  client->AppendString(str->substr(start, end - start + 1));
}

SetBitCmd::SetBitCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryString) {}

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
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

}  // namespace pikiwidb