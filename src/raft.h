/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <braft/raft.h>
#include <braft/util.h>
#include <gflags/gflags.h>
#include <memory>

namespace pikiwidb {

#define RAFT_DBID_LEN 32
#define RAFT_PORT_OFFSET 10

#define PRAFT PRaft::Instance()

class PRaft : public braft::StateMachine {
 public:
  PRaft() : _node(nullptr) {}

  ~PRaft() override = default;

  static PRaft& Instance();

  // Starts this node
  butil::Status Init(std::string& clust_id);
  butil::Status AddPeer(const std::string& peer);
  butil::Status RemovePeer(const std::string& peer);

  bool IsLeader() const;
  bool IsInitialized() const { return _node != nullptr; }

  void ShutDown();
  void Join();

 private:
  void on_apply(braft::Iterator& iter) override;
  void on_snapshot_save(braft::SnapshotWriter* writer, braft::Closure* done) override;
  int on_snapshot_load(braft::SnapshotReader* reader) override;

  void on_leader_start(int64_t term) override;
  void on_leader_stop(const butil::Status& status) override;

  void on_shutdown() override;
  void on_error(const ::braft::Error& e) override;
  void on_configuration_committed(const ::braft::Configuration& conf) override;
  void on_stop_following(const ::braft::LeaderChangeContext& ctx) override;
  void on_start_following(const ::braft::LeaderChangeContext& ctx) override;
  // end of @braft::StateMachine

 private:
  std::unique_ptr<braft::Node> _node;
  std::string _dbid;
};

}  // namespace pikiwidb