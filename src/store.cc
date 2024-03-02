/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "store.h"
#include <cassert>
#include <limits>
#include <string>
#include "config.h"
#include "log.h"
#include "multi.h"
namespace pikiwidb {

PStore& PStore::Instance() {
  static PStore store;
  return store;
}

void PStore::Init(int dbNum) {
  if (g_config.backend == kBackEndNone) {
    return;
  }

  backends_.resize(dbNum);

  if (g_config.backend == kBackEndRocksDB) {
    for (size_t i = 0; i < dbNum; ++i) {
      std::unique_ptr<storage::Storage> db = std::make_unique<storage::Storage>();
      storage::StorageOptions storage_options;
      storage_options.options.create_if_missing = true;
      storage_options.db_instance_num = g_config.db_instance_num;

      // options for CF
      storage_options.options.ttl = g_config.rocksdb_ttl_second;
      storage_options.options.periodic_compaction_seconds = g_config.rocksdb_periodic_second;

      PString dbpath = g_config.dbpath + std::to_string(i) + '/';

      storage::Status s = db->Open(storage_options, dbpath.data());
      if (!s.ok()) {
        assert(false);
      } else {
        INFO("Open RocksDB {} success", dbpath);
      }

      backends_[i] = std::move(db);
    }
  } else {
    // ERROR: unsupport backend
    return;
  }
}

}  // namespace pikiwidb
