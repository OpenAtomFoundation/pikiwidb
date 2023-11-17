/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_kv.h"
#include "pstd/pstd_string.h"
#include "store.h"

namespace pikiwidb {

GetCmd::GetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString) {}

bool GetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetCmd::DoCmd(PClient* client) {
  PObject* value;
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

IncrbyCmd::IncrbyCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool IncrbyCmd::DoInitial(PClient* client) {
  if (client->argv_.size() < 3) {
    client->SetRes(CmdRes::kWrongNum, "incrby");
    return false;
  }
  printf("logloglog");
  if (!(pstd::String2int(client->argv_[2].data(), client->argv_[2].size(), &by_))) {
    client->SetRes(CmdRes::kInvalidInt);
    return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

void IncrbyCmd::DoCmd(PClient* client) {
  PObject* value;
  PError err = PSTORE.Incrby(client->Key(), by_, &new_value_);
  switch (err) {
    case PError_type:
      client->SetRes(CmdRes::kInvalidInt);
      break;
    case PError_notExist:                 // key not exist, set a new value
      PSTORE.ClearExpire(client->Key());  // clear key's old ttl
      PSTORE.SetValue(client->Key(), PObject::CreateString(by_));
      client->SetRes(CmdRes::kOk);
      break;
    case PError_ok:
      client->AppendInteger(new_value_);
      break;
    default:
      client->SetRes(CmdRes::kErrOther, "incrby cmd error");
      break;
  }
}
}  // namespace pikiwidb