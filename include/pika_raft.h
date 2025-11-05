// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKA_RAFT_H_
#define PIKA_RAFT_H_

#include <memory>
#include <string>
#include <vector>

// Include praft.h to get complete RaftManager definition
// This must come before pika_server.h to avoid incomplete type errors
#include "praft/praft.h"

#include "include/acl.h"
#include "include/pika_command.h"

/*
 * Raft Commands
 */

// RAFT.CLUSTER [INIT|INFO] [args...]
// INIT: RAFT.CLUSTER INIT [peer1,peer2,peer3]  (peers optional, no peers = prepare for cluster expansion)
// INFO: RAFT.CLUSTER INFO [db_name]
class RaftClusterCmd : public Cmd {
 public:
  RaftClusterCmd(const std::string& name, int arity, uint32_t flag)
      : Cmd(name, arity, flag, static_cast<uint32_t>(AclCategory::ADMIN)) {}
  
  void Do() override;
  void Split(const HintKeys& hint_keys) override {};
  void Merge() override {};
  Cmd* Clone() override { return new RaftClusterCmd(*this); }

 private:
  enum class Operation { INIT, INFO, UNKNOWN };
  
  void DoInitial() override;
  void Clear() override {
    operation_ = Operation::UNKNOWN;
    db_name_.clear();
    args_.clear();
  }
  
  Operation operation_;
  std::string db_name_;
  std::vector<std::string> args_;
};

// RAFT.NODE ADD|REMOVE peer_address [db_name]
class RaftNodeCmd : public Cmd {
 public:
  RaftNodeCmd(const std::string& name, int arity, uint32_t flag)
      : Cmd(name, arity, flag, static_cast<uint32_t>(AclCategory::ADMIN)) {}
  
  void Do() override;
  void Split(const HintKeys& hint_keys) override {};
  void Merge() override {};
  Cmd* Clone() override { return new RaftNodeCmd(*this); }

 private:
  enum class Operation { ADD, REMOVE, UNKNOWN };
  
  void DoInitial() override;
  void Clear() override {
    operation_ = Operation::UNKNOWN;
    peer_addr_.clear();
    db_name_.clear();
  }
  
  Operation operation_;
  std::string peer_addr_;
  std::string db_name_;
};

#endif  // PIKA_RAFT_H_

