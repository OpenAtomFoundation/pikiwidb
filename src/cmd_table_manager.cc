/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_table_manager.h"
#include <memory>
#include "cmd_admin.h"
#include "cmd_keys.h"
#include "cmd_kv.h"

namespace pikiwidb {

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

  std::unique_ptr<BaseCmd> flushdbPtr = std::make_unique<FlushdbCmd>(kCmdNameFlushdb, 1);
  cmds_->insert(std::make_pair(kCmdNameFlushdb, std::move(flushdbPtr)));

  // keyspace
  std::unique_ptr<BaseCmd> delPtr = std::make_unique<DelCmd>(kCmdNameDel, -2);
  cmds_->insert(std::make_pair(kCmdNameDel, std::move(delPtr)));
  std::unique_ptr<BaseCmd> existsPtr = std::make_unique<ExistsCmd>(kCmdNameExists, 2);
  cmds_->insert(std::make_pair(kCmdNameExists, std::move(existsPtr)));

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