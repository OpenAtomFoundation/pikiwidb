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
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {             // = set command
      PSTORE.ClearExpire(client->argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(client->argv_[1], PObject::CreateString(client->argv_[2]));
      client->AppendInteger(static_cast<int64_t>(client->argv_[2].size()));
    } else {  // append string
      auto str = GetDecodedString(value);
      std::string old_value(str->c_str(), str->size());
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
      std::string ret_value(str->c_str(), str->size());
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
  std::vector<std::string> keys(client->argv_.begin(), client->argv_.end());
  client->keys_ = keys;
  client->keys_.erase(client->keys_.begin());
  return true;
}

void MgetCmd::DoCmd(PClient *client) {
  size_t valueSize = client->keys_.size();
  client->AppendArrayLen(static_cast<int64_t >(valueSize));
  for(const auto& k : client->keys_) {
    PObject* value;
    PError err = PSTORE.GetValueByType(k, value, PType_string);
    if (err == PError_notExist) {
      client->AppendContent("$-1");
    } else {
      auto str = GetDecodedString(value);
      std::string reply(str->c_str(), str->size());
      client->AppendContent(reply);
    }
  }
}

MSetCmd::MSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString){}

bool MSetCmd::DoInitial(PClient* client) {
  size_t argcSize = client->argv_.size();
  if (argcSize % 2 == 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameMset);
    return false;
  }
  client->kvs_.clear();
  for (size_t index = 1; index != argcSize; index += 2) {
    client->kvs_.emplace_back(client->argv_[index], client->argv_[index + 1]);
  }
  return true;
}

void MSetCmd::DoCmd(PClient* client) {
  std::vector<std::pair<std::string, std::string>>::const_iterator it;
  for (it = client->kvs_.begin(); it != client->kvs_.end(); it++) {
    PSTORE.ClearExpire(it->first);  // clear key's old ttl
    PSTORE.SetValue(it->first, PObject::CreateString(it->second));
  }
  client->SetRes(CmdRes::kOk);
}

}  // namespace pikiwidb
