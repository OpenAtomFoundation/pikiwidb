/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_table_manager.h"
#include "common.h"
#include "event_loop.h"
#include "io_thread_pool.h"
#include "tcp_connection.h"
#include "praft/praft.h"

#define kPIKIWIDB_VERSION "4.0.0"

namespace pikiwidb {
class PRaft;
}  // namespace pikiwidb

class PikiwiDB final {
 public:
  PikiwiDB() = default;
  ~PikiwiDB() = default;

  bool ParseArgs(int ac, char* av[]);
  const PString& GetConfigName() const { return cfg_file_; }

  bool Init();
  void Run();
  //  void Recycle();
  void Stop();

  void OnNewConnection(pikiwidb::TcpConnection* obj);

  pikiwidb::CmdTableManager& GetCmdTableManager();

 public:
  PString cfg_file_;
  uint16_t port_{0};
  PString log_level_;

  PString master_;
  uint16_t master_port_{0};

  static const uint32_t kRunidSize;

 private:
  pikiwidb::IOThreadPool worker_threads_;
  pikiwidb::IOThreadPool slave_threads_;
  pikiwidb::CmdTableManager cmd_table_manager_;
};

extern std::unique_ptr<PikiwiDB> g_pikiwidb;
