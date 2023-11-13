/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_kv.h"
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

AppendCmd::AppendCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool AppendCmd::DoInitial(PClient *client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void AppendCmd::DoCmd(PClient *client) {
  PObject* value;
  PError err = PSTORE.GetValueByType(client->Key(),value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {             // = set command
      PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendInteger(static_cast<int64_t>(client->argv_[2].size()));
    } else {  // append string
      auto str = GetDecodedString(value);
      std::string old_value(str->c_str(),str->size());
      std::string new_value = old_value + client->argv_[2];
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(new_value));
      client->AppendInteger(static_cast<int64_t>(new_value.size()));
    }
  }else {
    client->SetRes(CmdRes::kErrOther, "append cmd error");
  }
}

GetsetCmd::GetsetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool GetsetCmd::DoInitial(PClient *client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void GetsetCmd::DoCmd(PClient *client) {
  PObject* old_value;
  PError err = PSTORE.GetValueByType(client->Key(), old_value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {             // = set command
      PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendString("");
    } else {            // set new value
      auto str = GetDecodedString(old_value);
      std::string ret_value(str->c_str(),str->size());
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendString(ret_value);
    }
  }else {
    client->SetRes(CmdRes::kErrOther, "getset cmd error");
  }
}

MgetCmd::MgetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString){}

bool MgetCmd::DoInitial(PClient *client) {
  if (!CheckArg(ctx.argv_.size())) {
    ctx.SetRes(CmdRes::kWrongNum, kCmdNameMget);
    return false;
  }
  std::vector<std::string> keys(ctx.argv_.begin(), ctx.argv_.end());
  keys_ = keys;
  keys_.erase(keys_.begin());
  split_res_.resize(keys_.size());
  return true;
}

void MgetCmd::DoCmd(PClient *client) {

}

}  // namespace pikiwidb
