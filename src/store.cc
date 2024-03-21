/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <string>

#include "config.h"
#include "db.h"
#include "log.h"
#include "store.h"

namespace pikiwidb {

PStore& PStore::Instance() {
  static PStore store;
  return store;
}

void PStore::Init() {
  if (g_config.backend == kBackEndNone) {
    return;
  }

  dumpPath_ = g_config.dumppath;

  dbNum_ = g_config.databases;
  backends_.reserve(dbNum_);
  if (g_config.backend == kBackEndRocksDB) {
    for (int i = 0; i < dbNum_; i++) {
      auto db = std::make_unique<DB>(i, g_config.dbpath, dumpPath_);
      backends_.push_back(std::move(db));
    }
  } else {
    ERROR("unsupport backend!");
    return;
  }
}

void PStore::DoSameThingSpecificDB(const TaskContext task) {
  auto& type_ref = task.type;
  auto& dbs_ref = task.dbs;
  auto& argv_ref = task.argv;
  for (auto dbnum : dbs_ref) {
    if (dbnum >= dbNum_ || dbnum < 0) {
      continue ;
    }

    switch (type_ref) {
      case TaskType::kBgSave:
        auto& db = backends_[dbnum];
        db->CreateCheckpoint();
    }
  }
}

void PStore::FinishCheckpoint(bool sync) {
  for(auto& db : backends_) {
    db->FinishCheckpointDone(sync);
  }
}

}  // namespace pikiwidb
