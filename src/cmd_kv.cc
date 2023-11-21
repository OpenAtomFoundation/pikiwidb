/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_kv.h"
#include "store.h"
#include "string.h"
#include "./pstd/pstd_string.h"
#include <iostream>

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

BitOpCmd::BitOpCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryString) {}

bool BitOpCmd::DoInitial(PClient* client) {
  if (pstd::StringEqualCaseInsensitive(client->argv_[1],"and") &&
      pstd::StringEqualCaseInsensitive(client->argv_[1],"or") &&
      pstd::StringEqualCaseInsensitive(client->argv_[1],"not") &&
      pstd::StringEqualCaseInsensitive(client->argv_[1],"xor")) {
      client->SetRes(CmdRes::kSyntaxErr, "operation error");
      return false;
  }
  client->SetKey(client->argv_[1]);
  return true;
}

static PString StringBitOp(const std::vector<const PString*>& keys, BitOpCmd::BitOp op) {
  PString res;

  switch (op) {
    case BitOpCmd::BitOp_and:
    case BitOpCmd::BitOp_or:
    case BitOpCmd::BitOp_xor:
      for (auto k : keys) {
        PObject* val;
        if (PSTORE.GetValueByType(*k, val, PType_string) != PError_ok) {
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
          if (op == BitOpCmd::BitOp_and) {
            res[i] &= (*str)[i];
          } else if (op == BitOpCmd::BitOp_or) {
            res[i] |= (*str)[i];
          } else if (op == BitOpCmd::BitOp_xor) {
            res[i] ^= (*str)[i];
          }
        }
      }
      break;

    case BitOpCmd::BitOp_not: {
      assert(keys.size() == 1);
      PObject* val;
      if (PSTORE.GetValueByType(*keys[0], val, PType_string) != PError_ok) {
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

  std::vector<const PString*> keys;
  for (size_t i = 3; i < client->argv_.size(); ++i) {
    keys.push_back(&client->argv_[i]);
  }

  PError err = PError_param;
  PString res;

  if (client->Key().size() == 2) {
    if (strncasecmp(client->argv_[1].c_str(), "or", 2) == 0) {
      err = PError_ok;
      res = StringBitOp(keys, BitOp_or);
    }
  } else if (client->Key().size() == 3) {
    if (strncasecmp(client->argv_[1].c_str(), "xor", 3) == 0) {
      err = PError_ok;
      res = StringBitOp(keys, BitOp_xor);
    } else if (strncasecmp(client->argv_[1].c_str(), "and", 3) == 0) {
      err = PError_ok;
      res = StringBitOp(keys, BitOp_and);
    } else if (strncasecmp(client->argv_[1].c_str(), "not", 3) == 0) {
      if (client->argv_.size() == 4) {
        err = PError_ok;
        res = StringBitOp(keys, BitOp_not);
      }
    } else {
      ;
    }
  }

  if (err != PError_ok) {
    client->SetRes(CmdRes::kSyntaxErr);
  } else {
    PSTORE.SetValue(client->argv_[2], PObject::CreateString(res));
    client->SetRes(CmdRes::kOk,std::to_string(static_cast<long>(res.size())));
  }
  client->SetRes(CmdRes::kOk,std::to_string(static_cast<long>(res.size())));
}

}  // namespace pikiwidb