/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"
#include "store.h"

namespace pikiwidb {

HSetNxCmd::HSetNxCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryHash) {}

bool HSetNxCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HSetNxCmd::DoCmd(PClient* client) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(client->Key(), value, PType_hash);
  if (err != PError_ok && err != PError_notExist) {
    client->SetRes(CmdRes::kErrOther);
    return;
  }
  if (err == PError_notExist) {
    value = PSTORE.SetValue(client->Key(), PObject::CreateHash());
  }

  auto hash = value->CastHash();
  if (hash->insert(PHash::value_type(client->argv_[2], client->argv_[3])).second) {
    client->AppendInteger(1);
  } else {
    client->AppendInteger(0);
  }
  //  if (_set_hash_if_notexist(*hash, client->argv_[2], client->argv_[3])) {
  //    client->AppendInteger(1);
  //  } else {
  //    client->AppendInteger(0);
  //  }

  return;
}

}  // namespace pikiwidb
