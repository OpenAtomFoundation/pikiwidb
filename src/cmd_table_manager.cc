/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_table_manager.h"
#include <memory>
#include "cmd_admin.h"
#include "cmd_hash.h"
#include "cmd_keys.h"
#include "cmd_kv.h"

namespace pikiwidb {

#define ADD_COMMAND(cmd, argc)                                                      \
  do {                                                                              \
    std::unique_ptr<BaseCmd> ptr = std::make_unique<cmd##Cmd>(kCmdName##cmd, argc); \
    cmds_->insert(std::make_pair(kCmdName##cmd, std::move(ptr)));                   \
  } while (0)

CmdTableManager::CmdTableManager() {
  cmds_ = std::make_unique<CmdTable>();
  cmds_->reserve(300);
}

void CmdTableManager::InitCmdTable() {
  std::unique_lock wl(mutex_);

  // admin
  auto configPtr = std::make_unique<CmdConfig>(kCmdNameConfig, -2);
  configPtr->AddSubCmd(std::make_unique<CmdConfigGet>("get", -3));
  configPtr->AddSubCmd(std::make_unique<CmdConfigSet>("set", -4));
  cmds_->insert(std::make_pair(kCmdNameConfig, std::move(configPtr)));

  // server
  ADD_COMMAND(Flushdb, 1);

  // keyspace
  ADD_COMMAND(Del, -2);
  ADD_COMMAND(Exists, 2);

  // kv
  std::unique_ptr<BaseCmd> getPtr = std::make_unique<GetCmd>(kCmdNameGet, 2);
  cmds_->insert(std::make_pair(kCmdNameGet, std::move(getPtr)));
  std::unique_ptr<BaseCmd> setPtr = std::make_unique<SetCmd>(kCmdNameSet, -3);
  cmds_->insert(std::make_pair(kCmdNameSet, std::move(setPtr)));

  std::unique_ptr<BaseCmd> bitOpPtr = std::make_unique<BitOpCmd>(kCmdNameBitOp, -4);
  cmds_->insert(std::make_pair(kCmdNameBitOp, std::move(bitOpPtr)));
  std::unique_ptr<BaseCmd> appendPtr = std::make_unique<AppendCmd>(kCmdNameAppend, 3);
  cmds_->insert(std::make_pair(kCmdNameAppend, std::move(appendPtr)));
  std::unique_ptr<BaseCmd> getsetPtr = std::make_unique<GetsetCmd>(kCmdNameGetset, 3);
  cmds_->insert(std::make_pair(kCmdNameGetset, std::move(getsetPtr)));
  std::unique_ptr<BaseCmd> mgetPtr = std::make_unique<MgetCmd>(kCmdNameMget, -2);
  cmds_->insert(std::make_pair(kCmdNameMget, std::move(mgetPtr)));
  std::unique_ptr<BaseCmd> msetPtr = std::make_unique<MSetCmd>(kCmdNameMset, -3);
  cmds_->insert(std::make_pair(kCmdNameMset, std::move(msetPtr)));
  std::unique_ptr<BaseCmd> bitcountPtr = std::make_unique<BitCountCmd>(kCmdNameBitCount, -2);
  cmds_->insert(std::make_pair(kCmdNameBitCount, std::move(bitcountPtr)));
  std::unique_ptr<BaseCmd> incrPtr = std::make_unique<IncrCmd>(kCmdNameIncr, 2);
  cmds_->insert(std::make_pair(kCmdNameIncr, std::move(incrPtr)));
  std::unique_ptr<BaseCmd> incrbyPtr = std::make_unique<IncrbyCmd>(kCmdNameIncrby, 3);
  cmds_->insert(std::make_pair(kCmdNameIncrby, std::move(incrbyPtr)));
  std::unique_ptr<BaseCmd> strlenPtr = std::make_unique<StrlenCmd>(kCmdNameStrlen, 2);
  cmds_->insert(std::make_pair(kCmdNameStrlen, std::move(strlenPtr)));
  std::unique_ptr<BaseCmd> setexPtr = std::make_unique<SetexCmd>(kCmdNameSetex, 4);
  cmds_->insert(std::make_pair(kCmdNameSetex, std::move(setexPtr)));
  std::unique_ptr<BaseCmd> psetexPtr = std::make_unique<PsetexCmd>(kCmdNamePsetex, 4);
  cmds_->insert(std::make_pair(kCmdNamePsetex, std::move(psetexPtr)));
  std::unique_ptr<BaseCmd> setnxPtr = std::make_unique<SetnxCmd>(kCmdNameSetnx, 3);
  cmds_->insert(std::make_pair(kCmdNameSetnx, std::move(setnxPtr)));
  std::unique_ptr<BaseCmd> getbitPtr = std::make_unique<GetBitCmd>(kCmdNameGetBit, 3);
  cmds_->insert(std::make_pair(kCmdNameGetBit, std::move(getbitPtr)));

  ADD_COMMAND(Get, 2);
  ADD_COMMAND(Set, -3);
  ADD_COMMAND(MGet, -2);
  ADD_COMMAND(MSet, -3);
  ADD_COMMAND(GetSet, 3);
  ADD_COMMAND(SetNX, 3);
  ADD_COMMAND(Append, 3);
  ADD_COMMAND(Strlen, 2);
  ADD_COMMAND(Incrby, 3);
  ADD_COMMAND(Decrby, 3);
  ADD_COMMAND(IncrbyFloat, 3);
  ADD_COMMAND(SetEx, 4);
  ADD_COMMAND(PSetEx, 4);
  ADD_COMMAND(BitOp, -4);
  ADD_COMMAND(BitCount, -2);
  ADD_COMMAND(GetBit, 3);

  // hash
  ADD_COMMAND(HSet, -4);
  ADD_COMMAND(HGet, 3);
  ADD_COMMAND(HMSet, -4);
  ADD_COMMAND(HMGet, -3);
  ADD_COMMAND(HGetAll, 2);
  ADD_COMMAND(HKeys, 2);
  ADD_COMMAND(HLen, 2);
  ADD_COMMAND(HStrLen, 3);
}

std::pair<BaseCmd*, CmdRes::CmdRet> CmdTableManager::GetCommand(const std::string& cmdName, PClient* client) {
  std::shared_lock rl(mutex_);

  auto cmd = cmds_->find(cmdName);

  if (cmd == cmds_->end()) {
    return std::pair(nullptr, CmdRes::kSyntaxErr);
  }

  if (cmd->second->HasSubCommand()) {
    if (client->argv_.size() < 2) {
      return std::pair(nullptr, CmdRes::kInvalidParameter);
    }
    return std::pair(cmd->second->GetSubCmd(client->argv_[1]), CmdRes::kSyntaxErr);
  }
  return std::pair(cmd->second.get(), CmdRes::kSyntaxErr);
}

bool CmdTableManager::CmdExist(const std::string& cmd) const {
  std::shared_lock rl(mutex_);
  return cmds_->find(cmd) != cmds_->end();
}

uint32_t CmdTableManager::GetCmdId() { return ++cmdId_; }

}  // namespace pikiwidb
