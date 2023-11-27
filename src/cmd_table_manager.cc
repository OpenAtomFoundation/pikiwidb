/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_table_manager.h"
#include <memory>
#include "cmd_admin.h"
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

  // kv
  std::unique_ptr<BaseCmd> getPtr = std::make_unique<GetCmd>(kCmdNameGet, 2);
  cmds_->insert(std::make_pair(kCmdNameGet, std::move(getPtr)));
  std::unique_ptr<BaseCmd> setPtr = std::make_unique<SetCmd>(kCmdNameSet, -3);
  cmds_->insert(std::make_pair(kCmdNameSet, std::move(setPtr)));
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