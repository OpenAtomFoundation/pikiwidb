/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "common.h"
#include "tcp_connection.h"

#include <set>
#include <unordered_map>
#include <unordered_set>
#include "proto_parser.h"
#include "replication.h"

namespace pikiwidb {

enum ClientFlag {
  ClientFlag_multi = 0x1,
  ClientFlag_dirty = 0x1 << 1,
  ClientFlag_wrongExec = 0x1 << 2,
  ClientFlag_master = 0x1 << 3,
};

class DB;
struct PSlaveInfo;

class PClient : public std::enable_shared_from_this<PClient> {
 public:
  PClient() = delete;
  explicit PClient(TcpConnection* obj);

  int HandlePackets(pikiwidb::TcpConnection*, const char*, int);

  void OnConnect();

  const std::string& PeerIP() const;
  int PeerPort() const;

  bool SendPacket(const std::string& buf);
  bool SendPacket(const void* data, size_t size);
  bool SendPacket(UnboundedBuffer& data);
  bool SendPacket(const evbuffer_iovec* iovecs, size_t nvecs);

  void Close();

  bool SelectDB(int db);
  static PClient* Current();

  // multi
  void SetFlag(unsigned flag) { flag_ |= flag; }
  void ClearFlag(unsigned flag) { flag_ &= ~flag; }
  bool IsFlagOn(unsigned flag) { return flag_ & flag; }
  void FlagExecWrong() {
    if (IsFlagOn(ClientFlag_multi)) {
      SetFlag(ClientFlag_wrongExec);
    }
  }

  bool Watch(int dbno, const PString& key);
  bool NotifyDirty(int dbno, const PString& key);
  bool Exec();
  void ClearMulti();
  void ClearWatch();

  // pubsub
  std::size_t Subscribe(const PString& channel) { return channels_.insert(channel).second ? 1 : 0; }

  std::size_t UnSubscribe(const PString& channel) { return channels_.erase(channel); }

  std::size_t PSubscribe(const PString& channel) { return pattern_channels_.insert(channel).second ? 1 : 0; }

  std::size_t PUnSubscribe(const PString& channel) { return pattern_channels_.erase(channel); }

  const std::unordered_set<PString>& GetChannels() const { return channels_; }
  const std::unordered_set<PString>& GetPatternChannels() const { return pattern_channels_; }
  std::size_t ChannelCount() const { return channels_.size(); }
  std::size_t PatternChannelCount() const { return pattern_channels_.size(); }

  bool WaitFor(const PString& key, const PString* target = nullptr);

  const std::unordered_set<PString> WaitingKeys() const { return waiting_keys_; }
  void ClearWaitingKeys() { waiting_keys_.clear(), target_.clear(); }
  const PString& GetTarget() const { return target_; }

  void SetName(const PString& name) { name_ = name; }
  const PString& GetName() const { return name_; }

  void SetSlaveInfo();
  PSlaveInfo* GetSlaveInfo() const { return slave_info_.get(); }

  static void AddCurrentToMonitor();
  static void FeedMonitors(const std::vector<PString>& params);

  void SetAuth() { auth_ = true; }
  bool GetAuth() const { return auth_; }
  void RewriteCmd(std::vector<PString>& params) { parser_.SetParams(params); }

 private:
  std::shared_ptr<TcpConnection> getTcpConnection() const { return tcp_connection_.lock(); }
  int handlePacket(pikiwidb::TcpConnection*, const char*, int);
  int handlePacketNew(const std::vector<std::string>& params, const std::string& cmd);
  int processInlineCmd(const char*, size_t, std::vector<PString>&);
  void reset();
  bool isPeerMaster() const;
  int UniqueId() const;

  // TcpConnection's life is undetermined, so use weak ptr for safety.
  std::weak_ptr<TcpConnection> tcp_connection_;

  PProtoParser parser_;
  UnboundedBuffer reply_;

  int db_ = -1;

  std::unordered_set<PString> channels_;
  std::unordered_set<PString> pattern_channels_;

  unsigned flag_;
  std::unordered_map<int, std::unordered_set<PString> > watch_keys_;
  std::vector<std::vector<PString> > queue_cmds_;

  // blocked list
  std::unordered_set<PString> waiting_keys_;
  PString target_;

  // slave info from master view
  std::unique_ptr<PSlaveInfo> slave_info_;

  // name
  std::string name_;

  // auth
  bool auth_ = false;
  time_t last_auth_ = 0;

  static thread_local PClient* s_current;
};

}  // namespace pikiwidb
