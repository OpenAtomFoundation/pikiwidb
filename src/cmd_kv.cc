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
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString) {}

bool GetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kSyntaxErr, "get key error");
    }
    return;
  }
  auto str = GetDecodedString(value);
  std::string reply(str->c_str(), str->size());
  client->AppendString(reply);
}

SetCmd::SetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool SetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetCmd::DoCmd(PClient* client) {
  PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
  PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
  client->SetRes(CmdRes::kOk);
}

AppendCmd::AppendCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool AppendCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void AppendCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {            // = set command
      PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendInteger(static_cast<int64_t>(client->argv_[2].size()));
    } else {
      client->SetRes(CmdRes::kErrOther, "append cmd error");
    }
    return;
  }
  auto str = GetDecodedString(value);
  std::string old_value(str->c_str(), str->size());
  std::string new_value = old_value + client->argv_[2];
  PSTORE.SetValue(client->argv_[1], PObject::CreateString(new_value));
  client->AppendInteger(static_cast<int64_t>(new_value.size()));
}

GetSetCmd::GetSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool GetSetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetSetCmd::DoCmd(PClient* client) {
  PObject* old_value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), old_value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {            // = set command
      PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendString("");
    } else {
      client->SetRes(CmdRes::kErrOther, "getset cmd error");
    }
    return;
  }
  auto str = GetDecodedString(old_value);
  PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
  PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
  client->AppendString(*str);
}

MGetCmd::MGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString) {}

bool MGetCmd::DoInitial(PClient* client) {
  std::vector<std::string> keys(client->argv_.begin(), client->argv_.end());
  keys.erase(keys.begin());
  client->SetKey(keys);
  return true;
}

void MGetCmd::DoCmd(PClient* client) {
  size_t valueSize = client->Keys().size();
  client->AppendArrayLen(static_cast<int64_t>(valueSize));
  for (const auto& k : client->Keys()) {
    PObject* value = nullptr;
    PError err = PSTORE.GetValueByType(k, value, PType_string);
    if (err == PError_notExist) {
      client->AppendStringLen(-1);
    } else {
      auto str = GetDecodedString(value);
      std::string reply(str->c_str(), str->size());
      client->AppendStringLen(static_cast<int64_t>(reply.size()));
      client->AppendContent(reply);
    }
  }
}

MSetCmd::MSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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
  int valueIndex = 2;
  for (const auto& it : client->Keys()) {
    PSTORE.ClearExpire(it);  // clear key's old ttl
    PSTORE.SetValue(it, PObject::CreateString(client->argv_[valueIndex]));
    valueIndex += 2;
  }
  client->SetRes(CmdRes::kOk);
}

BitCountCmd::BitCountCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString) {}

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
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->argv_[1], value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      client->AppendInteger(0);
    } else {
      client->SetRes(CmdRes::kErrOther, "bitcount get key error");
    }
    return;
  }

  int64_t start_offset_;
  int64_t end_offset_;
  if (pstd::String2int(client->argv_[2], &start_offset_) == 0 ||
      pstd::String2int(client->argv_[3], &end_offset_) == 0) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  auto str = GetDecodedString(value);
  auto value_length = static_cast<int64_t>(str->size());
  if (start_offset_ < 0) {
    start_offset_ += value_length;
  }
  if (end_offset_ < 0) {
    end_offset_ += value_length;
  }
  if (start_offset_ < 0) {
    start_offset_ = 0;
  }
  if (end_offset_ < 0) {
    end_offset_ = 0;
  }
  if (end_offset_ >= value_length) {
    end_offset_ = value_length - 1;
  }
  size_t count = 0;
  if (end_offset_ >= start_offset_) {
    count = BitCount(reinterpret_cast<const uint8_t*>(str->data()) + start_offset_, end_offset_ - start_offset_ + 1);
  }
  client->AppendInteger(static_cast<int64_t>(count));
}

BitOpCmd::BitOpCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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

static std::string StringBitOp(const std::vector<std::string>& keys, BitOpCmd::BitOp op) {
  PString res;

  switch (op) {
    case BitOpCmd::kBitOpAnd:
    case BitOpCmd::kBitOpOr:
    case BitOpCmd::kBitOpXor:
      for (auto k : keys) {
        PObject* val = nullptr;
        if (PSTORE.GetValueByType(k, val, PType_string) != PError_ok) {
          continue;
        }

        auto str = GetDecodedString(val);
        if (res.empty()) {
          res = *str;
          continue;
        }

        if (str->size() > res.size()) {
          res.resize(str->size());
        }

        for (size_t i = 0; i < str->size(); ++i) {
          if (op == BitOpCmd::kBitOpAnd) {
            res[i] &= (*str)[i];
          } else if (op == BitOpCmd::kBitOpOr) {
            res[i] |= (*str)[i];
          } else if (op == BitOpCmd::kBitOpXor) {
            res[i] ^= (*str)[i];
          }
        }
      }
      break;

    case BitOpCmd::kBitOpNot: {
      assert(keys.size() == 1);
      PObject* val = nullptr;
      if (PSTORE.GetValueByType(keys[0], val, PType_string) != PError_ok) {
        break;
      }

      auto str = GetDecodedString(val);
      res.resize(str->size());

      for (size_t i = 0; i < str->size(); ++i) {
        res[i] = ~(*str)[i];
      }

      break;
    }

    default:
      break;
  }

  return res;
}

void BitOpCmd::DoCmd(PClient* client) {
  std::vector<std::string> keys;
  for (size_t i = 3; i < client->argv_.size(); ++i) {
    keys.push_back(client->argv_[i]);
  }

  PError err = PError_param;
  PString res;

  if (client->Key().size() == 2) {
    if (pstd::StringEqualCaseInsensitive(client->argv_[1], "or")) {
      err = PError_ok;
      res = StringBitOp(keys, kBitOpOr);
    }
  } else if (client->Key().size() == 3) {
    if (pstd::StringEqualCaseInsensitive(client->argv_[1], "xor")) {
      err = PError_ok;
      res = StringBitOp(keys, kBitOpXor);
    } else if (pstd::StringEqualCaseInsensitive(client->argv_[1], "and")) {
      err = PError_ok;
      res = StringBitOp(keys, kBitOpAnd);
    } else if (pstd::StringEqualCaseInsensitive(client->argv_[1], "not")) {
      if (client->argv_.size() == 4) {
        err = PError_ok;
        res = StringBitOp(keys, kBitOpNot);
      }
    }
  }

  if (err != PError_ok) {
    client->SetRes(CmdRes::kSyntaxErr);
  } else {
    PSTORE.SetValue(client->argv_[2], PObject::CreateString(res));
    client->SetRes(CmdRes::kOk, std::to_string(static_cast<long>(res.size())));
  }
  client->SetRes(CmdRes::kOk, std::to_string(static_cast<long>(res.size())));
}

StrlenCmd::StrlenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString) {}

bool StrlenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void StrlenCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_string);

  switch (err) {
    case PError_ok: {
      auto str = GetDecodedString(value);
      size_t len = str->size();
      client->AppendInteger(static_cast<int64_t>(len));
      break;
    }
    case PError_notExist: {
      client->AppendInteger(0);
      break;
    }
    default: {
      client->SetRes(CmdRes::kErrOther, "error other");
      break;
    }
  }
}

SetExCmd::SetExCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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
  PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[3]));
  int64_t sec = 0;
  pstd::String2int(client->argv_[2], &sec);
  PSTORE.SetExpire(client->argv_[1], pstd::UnixMilliTimestamp() + sec * 1000);
  client->SetRes(CmdRes::kOk);
}

PSetExCmd::PSetExCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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
  PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[3]));
  int64_t msec = 0;
  pstd::String2int(client->argv_[2], &msec);
  PSTORE.SetExpire(client->argv_[1], pstd::UnixMilliTimestamp() + msec);
  client->SetRes(CmdRes::kOk);
}

IncrbyCmd::IncrbyCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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
  int64_t new_value = 0;
  int64_t by_ = 0;
  pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by_);
  PError err = PSTORE.Incrby(client->Key(), by_, &new_value);
  switch (err) {
    case PError_type:
      client->SetRes(CmdRes::kInvalidInt);
      break;
    case PError_notExist:                 // key not exist, set a new value
      PSTORE.ClearExpire(client->Key());  // clear key's old ttl
      PSTORE.SetValue(client->Key(), PObject::CreateString(by_));
      client->AppendInteger(by_);
      break;
    case PError_ok:
      client->AppendInteger(new_value);
      break;
    default:
      client->SetRes(CmdRes::kErrOther, "incrby cmd error");
      break;
  }
}

IncrbyFloatCmd::IncrbyFloatCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

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
  std::string new_value;
  PError err = PSTORE.Incrbyfloat(client->argv_[1], client->argv_[2], &new_value);
  switch (err) {
    case PError_type:
      client->SetRes(CmdRes::kInvalidFloat);
      break;
    case PError_notExist:                 // key not exist, set a new value
      PSTORE.ClearExpire(client->Key());  // clear key's old ttl
      PSTORE.SetValue(client->Key(), PObject::CreateString(client->argv_[2]));
      client->AppendString(client->argv_[2]);
      break;
    case PError_ok:
      client->AppendString(new_value);
      break;
    default:
      client->SetRes(CmdRes::kErrOther, "incrbyfloat cmd error");
      break;
  }
}

SetNXCmd::SetNXCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool SetNXCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void SetNXCmd::DoCmd(PClient* client) {
  int iSuccess = 1;
  PObject* value = nullptr;
  PError err = PSTORE.GetValue(client->argv_[1], value);
  if (err == PError_notExist) {
    PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
    PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
    client->AppendInteger(iSuccess);
  } else {
    client->AppendInteger(!iSuccess);
  }
}

GetBitCmd::GetBitCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool GetBitCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetBitCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_string);
  if (err != PError_ok) {
    client->SetRes(CmdRes::kErrOther);
    return;
  }

  long offset = 0;
  if (!Strtol(client->argv_[2].c_str(), client->argv_[2].size(), &offset)) {
    client->SetRes(CmdRes::kInvalidInt);
    return;
  }

  auto str = GetDecodedString(value);
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

  return;
}

}  // namespace pikiwidb
