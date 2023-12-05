/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_keys.h"
#include "store.h"

namespace pikiwidb {

DelCmd::DelCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsWrite, AclCategoryWrite | AclCategoryKeyspace) {}

bool DelCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void DelCmd::DoCmd(PClient* client) {
  if (PSTORE.DeleteKey(client->Key())) {
    PSTORE.ClearExpire(client->Key());
    client->AppendInteger(1);
  } else {
    client->AppendInteger(0);
  }
}

ExistsCmd::ExistsCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, CmdFlagsReadonly, AclCategoryRead | AclCategoryKeyspace) {}

bool ExistsCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void ExistsCmd::DoCmd(PClient* client) {
  if (PSTORE.ExistsKey(client->Key())) {
    client->AppendInteger(1);
  } else {
    client->AppendInteger(0);
  }
}

}  // namespace pikiwidb