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

void PStore::Init(int dbNum) {
  if (g_config.backend == kBackEndNone) {
    return;
  }

  backends_.reserve(dbNum);

  if (g_config.backend == kBackEndRocksDB) {
    for (int i = 0; i < dbNum; i++) {
      auto db = std::make_unique<DB>(i, g_config.dbpath);
      backends_.push_back(std::move(db));
    }
  } else {
    ERROR("unsupport backend!");
  }
}

}  // namespace pikiwidb
