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

bool GetCmd::DoInitial(CmdContext& ctx) {
  if (!CheckArg(ctx.argv_.size())) {
    ctx.SetRes(CmdRes::kWrongNum, kCmdNameGet);
    return false;
  }
  ctx.key_ = ctx.argv_[1];
  return true;
}

void GetCmd::DoCmd(CmdContext& ctx) {
  PObject* value;
  PError err = PSTORE.GetValueByType(ctx.key_, value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {
      ctx.AppendString("");
    } else {
      ctx.SetRes(CmdRes::kSyntaxErr, "get key error");
    }
    return;
  }
  auto str = GetDecodedString(value);
  std::string reply(str->c_str(), str->size());
  ctx.AppendString(reply);
}

SetCmd::SetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool SetCmd::DoInitial(CmdContext& ctx) {
  if (!CheckArg(ctx.argv_.size())) {
    ctx.SetRes(CmdRes::kWrongNum, kCmdNameSet);
    return false;
  }
  ctx.key_ = ctx.argv_[1];
  return true;
}

void SetCmd::DoCmd(CmdContext& ctx) {
  PSTORE.ClearExpire(ctx.argv_[1]);  // clear key's old ttl
  PSTORE.SetValue(ctx.argv_[1], PObject::CreateString(ctx.argv_[2]));
  ctx.SetRes(CmdRes::kOk);
}

AppendCmd::AppendCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool AppendCmd::DoInitial(CmdContext& ctx) {
  if (!CheckArg(ctx.argv_.size())) {
    ctx.SetRes(CmdRes::kWrongNum, kCmdNameAppend);
    return false;
  }
  ctx.key_ = ctx.argv_[1];
  return true;
}

void AppendCmd::DoCmd(CmdContext& ctx) {
  PObject* value;
  PError err = PSTORE.GetValueByType(ctx.key_, value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {             // = set command
      PSTORE.ClearExpire(ctx.argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(ctx.argv_[1], PObject::CreateString(ctx.argv_[2]));
      ctx.AppendInteger(static_cast<int64_t>(ctx.argv_[2].size()));
    } else {  // append string
      auto str = GetDecodedString(value);
      std::string old_value(str->c_str(),str->size());
      std::string new_value = old_value + ctx.argv_[2];
      PSTORE.SetValue(ctx.argv_[1], PObject::CreateString(new_value));
      ctx.AppendInteger(static_cast<int64_t>(new_value.size()));
    }
  }else {
    ctx.SetRes(CmdRes::kErrOther, "append cmd error");
  }
}

GetsetCmd::GetsetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool GetsetCmd::DoInitial(CmdContext& ctx) {
  if (!CheckArg(ctx.argv_.size())) {
    ctx.SetRes(CmdRes::kWrongNum, kCmdNameGetset);
    return false;
  }
  ctx.key_ = ctx.argv_[1];
  return true;
}

void GetsetCmd::DoCmd(CmdContext& ctx) {
  PObject* old_value;
  PError err = PSTORE.GetValueByType(ctx.key_, old_value, PType_string);
  if (err != PError_ok) {
    if (err == PError_notExist) {             // = set command
      PSTORE.ClearExpire(ctx.argv_[1]);  // clear key's old ttl
      PSTORE.SetValue(ctx.argv_[1], PObject::CreateString(ctx.argv_[2]));
      ctx.AppendString("");
    } else {            // set new value
      auto str = GetDecodedString(old_value);
      std::string ret_value(str->c_str(),str->size());
      PSTORE.SetValue(ctx.argv_[1], PObject::CreateString(ctx.argv_[2]));
      ctx.AppendString(ret_value);
    }
  }else {
    ctx.SetRes(CmdRes::kErrOther, "getset cmd error");
  }
}

MgetCmd::MgetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryString){}

bool MgetCmd::DoInitial(CmdContext& ctx) {
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

void MgetCmd::DoCmd(CmdContext& ctx) {

}

}  // namespace pikiwidb
