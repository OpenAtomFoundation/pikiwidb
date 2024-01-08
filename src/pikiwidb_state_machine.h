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

// Implementation of example::Block as a braft::StateMachine.
class PikiwidbStateMachine : public braft::StateMachine {
 public:
  PikiwidbStateMachine() : _node(nullptr), _leader_term(-1) {}
  ~PikiwidbStateMachine() override = default;

  // Starts this node
  int start();
  bool is_leader() const;

  // Shut this node down.
  void shutdown();

  // Blocking this thread until the node is eventually down.
  void join();

 private:
  // @braft::StateMachine
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
  butil::atomic<int64_t> _leader_term;
};
