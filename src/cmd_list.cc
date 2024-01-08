/*
* Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_list.h"

#include <iostream>
#include <memory>
#include <utility>
#include "store.h"

namespace pikiwidb {

LPushCmd::LPushCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryList) {}
bool LPushCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}
void LPushCmd::DoCmd(PClient* client) {
  PObject *value = nullptr;
  //todo :need to change code after issue 114  becase this function should return an error if the key type does not match
  //@578223592
  PError err = PSTORE.GetValueByType(client->Key(),value,kPTypeList);
  if(err != kPErrorOK ) {
    if(err != kPErrorNotExist) {
      client->SetRes(CmdRes::kSyntaxErr,"lpush cmd error");
    }
    // if this key not exist , create it
    value = PSTORE.SetValue(client->Key(),PObject::CreateList());
  }
  auto desList = value->CastList();
  for(int i =2; i < client->argv_.size(); ++i) {
    desList->emplace_front(client->argv_[i]);
  }
  for(auto item :*desList) {
    std::cout<<item<<" ";
  }std::cout<<"\n";
  client->AppendInteger(desList->size());
}

}  // namespace pikiwidb


