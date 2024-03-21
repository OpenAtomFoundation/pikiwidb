/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_admin.h"
#include "pstd/env.h"
#include "store.h"

namespace pikiwidb {

CmdConfig::CmdConfig(const std::string& name, int arity) : BaseCmdGroup(name, kCmdFlagsAdmin, kAclCategoryAdmin) {}

bool CmdConfig::HasSubCommand() const { return true; }

CmdConfigGet::CmdConfigGet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryAdmin) {}

bool CmdConfigGet::DoInitial(PClient* client) { return true; }

void CmdConfigGet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

CmdConfigSet::CmdConfigSet(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin, kAclCategoryAdmin) {}

bool CmdConfigSet::DoInitial(PClient* client) { return true; }

void CmdConfigSet::DoCmd(PClient* client) { client->AppendString("config cmd in development"); }

FlushdbCmd::FlushdbCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryAdmin) {}

bool FlushdbCmd::DoInitial(PClient* client) { return true; }

void FlushdbCmd::DoCmd(PClient* client) {
  int currentDBIndex = client->GetCurrentDB();
  PSTORE.GetBackend(currentDBIndex).reset();

  std::string db_path = g_config.dbpath + std::to_string(currentDBIndex);
  std::string path_temp = db_path;
  path_temp.append("_deleting/");
  pstd::RenameFile(db_path, path_temp);

  PSTORE.GetBackend(currentDBIndex) = std::make_unique<storage::Storage>();
  storage::StorageOptions storage_options;
  storage_options.options.create_if_missing = true;
  storage_options.db_instance_num = g_config.db_instance_num;
  storage_options.options.ttl = g_config.rocksdb_ttl_second;
  storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;

  storage::Status s = PSTORE.GetBackend(currentDBIndex)->Open(storage_options, db_path.data());
  assert(s.ok());
  pstd::DeleteDir(path_temp);
  client->SetRes(CmdRes::kOK);
}

FlushallCmd::FlushallCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryAdmin) {}

bool FlushallCmd::DoInitial(PClient* client) { return true; }

void FlushallCmd::DoCmd(PClient* client) {
  for (size_t i = 0; i < g_config.databases; ++i) {
    PSTORE.GetBackend(i).reset();

    std::string db_path = g_config.dbpath + std::to_string(i);
    std::string path_temp = db_path;
    path_temp.append("_deleting/");
    pstd::RenameFile(db_path, path_temp);

    PSTORE.GetBackend(i) = std::make_unique<storage::Storage>();
    storage::StorageOptions storage_options;
    storage_options.options.create_if_missing = true;
    storage_options.db_instance_num = g_config.db_instance_num;
    storage_options.options.ttl = g_config.rocksdb_ttl_second;
    storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;

    storage::Status s = PSTORE.GetBackend(i)->Open(storage_options, db_path.data());
    assert(s.ok());
    pstd::DeleteDir(path_temp);
  }
  client->SetRes(CmdRes::kOK);
}

SelectCmd::SelectCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsAdmin | kCmdFlagsReadonly, kAclCategoryAdmin) {}

bool SelectCmd::DoInitial(PClient* client) { return true; }

void SelectCmd::DoCmd(PClient* client) {
  int index = atoi(client->argv_[1].c_str());
  if (index < 0 || index >= g_config.databases) {
    client->SetRes(CmdRes::kInvalidIndex, kCmdNameSelect + " DB index is out of range");
    return;
  }
  client->SetCurrentDB(index);
  client->SetRes(CmdRes::kOK);
}

}  // namespace pikiwidb