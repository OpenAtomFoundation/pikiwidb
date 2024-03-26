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

  backends_.reserve(dbNum_);

  dbNum_ = g_config.databases;
  backends_.reserve(dbNum_);
  if (g_config.backend == kBackEndRocksDB) {
    for (int i = 0; i < dbNum_; i++) {
      auto db = std::make_unique<DB>(i, g_config.dbpath);
      backends_.push_back(std::move(db));
    }
  } else {
    ERROR("unsupport backend!");
  }
}

void PStore::Clear() { backends_.clear(); }

void PStore::DoSomeThingSpecificDB(const TasksVector tasks) {
  std::for_each(tasks.begin(), tasks.end(), [this](const auto& task) {
    switch (task.type) {
      case kCheckpoint:
        if (task.db < 0 || task.db >= dbNum_) {
          WARN("The database index is out of range.");
          return;
        }
        auto& db = backends_[task.db];
        if (auto s = task.args.find(kCheckpointPath); s == task.args.end()) {
          WARN("The critical parameter 'path' is missing in the checkpoint.");
          return;
        }
        auto path = task.args.find(kCheckpointPath)->second;
        trimSlash(path);
        db->CreateCheckpoint(path);
        break;
    };
  });
}

void PStore::WaitForCheckpointDone() {
  for (auto& db : backends_) {
    db->WaitForCheckpointDone();
  }
}

void PStore::trimSlash(std::string& dirName) {
  while (dirName.back() == '/') {
    dirName.pop_back();
  }
}

}  // namespace pikiwidb
