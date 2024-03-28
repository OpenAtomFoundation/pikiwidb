/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include "common.h"
#include "net/unbounded_buffer.h"
#include "net/util.h"
#include "pstd/memory_file.h"

namespace pikiwidb {

template <typename DEST>
inline void WriteBulkString(const char* str, size_t strLen, DEST& dst) {
  char tmp[32];
  size_t n = snprintf(tmp, sizeof tmp, "$%lu\r\n", strLen);

  dst.Write(tmp, n);
  dst.Write(str, strLen);
  dst.Write("\r\n", 2);
}

template <typename DEST>
inline void WriteBulkString(const PString& str, DEST& dst) {
  WriteBulkString(str.data(), str.size(), dst);
}

template <typename DEST>
inline void WriteMultiBulkLong(long val, DEST& dst) {
  char tmp[32];
  size_t n = snprintf(tmp, sizeof tmp, "*%lu\r\n", val);
  dst.Write(tmp, n);
}

template <typename DEST>
inline void WriteBulkLong(long val, DEST& dst) {
  char tmp[32];
  size_t n = snprintf(tmp, sizeof tmp, "%lu", val);

  WriteBulkString(tmp, n, dst);
}

template <typename DEST>
inline void SaveCommand(const std::vector<PString>& params, DEST& dst) {
  WriteMultiBulkLong(params.size(), dst);

  for (const auto& s : params) {
    WriteBulkString(s, dst);
  }
}

// master side
enum PSlaveState {
  kPSlaveStateNone,
  kPSlaveStateWaitBgsaveStart,  // 有非sync的bgsave进行 要等待
  kPSlaveStateWaitBgsaveEnd,    // sync bgsave正在进行
  // PSlaveState_send_rdb, // 这个slave在接受rdb文件
  kPSlaveStateOnline,
};

struct PSlaveInfo {
  PSlaveState state;
  unsigned short listenPort;  // slave listening port

  PSlaveInfo() : state(kPSlaveStateNone), listenPort(0) {}
};

// slave side
enum PReplState {
  kPReplStateNone,
  kPReplStateConnecting,
  kPReplStateConnected,
  kPReplStateWaitAuth,      // wait auth to be confirmed
  kPReplStateWaitReplconf,  // wait replconf to be confirmed
  kPReplStateWaitRdb,       // wait to recv rdb file
  kPReplStateOnline,
};

struct PMasterInfo {
  SocketAddr addr;
  PReplState state;
  time_t downSince;

  // For recv rdb
  std::size_t rdbSize;
  std::size_t rdbRecved;

  PMasterInfo() {
    state = kPReplStateNone;
    downSince = 0;
    rdbSize = std::size_t(-1);
    rdbRecved = 0;
  }
};

// tmp filename
const char* const slaveRdbFile = "slave.rdb";

class PClient;

class PReplication {
 public:
  static PReplication& Instance();

  PReplication(const PReplication&) = delete;
  void operator=(const PReplication&) = delete;

  void Cron();

  // master side
  bool IsBgsaving() const;
  bool HasAnyWaitingBgsave() const;
  void AddSlave(PClient* cli);
  void TryBgsave();
  bool StartBgsave();
  void OnStartBgsave();
  void OnRdbSaveDone();
  void SendToSlaves(const std::vector<PString>& params);

  // slave side
  void SaveTmpRdb(const char* data, std::size_t& len);
  void SetMaster(const std::shared_ptr<PClient>& cli);
  void SetMasterState(PReplState s);
  void SetMasterAddr(const char* ip, unsigned short port);
  void SetRdbSize(std::size_t s);
  PReplState GetMasterState() const;
  SocketAddr GetMasterAddr() const;
  std::size_t GetRdbSize() const;

  // info command
  void OnInfoCommand(UnboundedBuffer& res);

 private:
  PReplication();
  void onStartBgsave(bool succ);

  // master side
  bool bgsaving_;
  UnboundedBuffer buffer_;
  std::list<std::weak_ptr<PClient> > slaves_;

  // slave side
  PMasterInfo masterInfo_;
  std::weak_ptr<PClient> master_;
  pstd::OutputMemoryFile rdb_;
};

}  // namespace pikiwidb

#define PREPL pikiwidb::PReplication::Instance()
