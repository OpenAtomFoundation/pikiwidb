// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_hyperloglog.h"
#include "include/pika_client_conn.h"
#include "include/pika_slot_command.h"
#include "storage/include/storage/storage.h"
#include "storage/include/storage/batch.h"

void PfAddCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, kCmdNamePfAdd);
    return;
  }
  if (argv_.size() > 1) {
    key_ = argv_[1];
    size_t pos = 2;
    while (pos < argv_.size()) {
      values_.push_back(argv_[pos++]);
    }
  }
}

void PfAddCmd::Do() {
  update_ = false;
  storage::CommitCallback callback = nullptr;
  
  if (ShouldUseAsyncMode()) {
    auto self = std::static_pointer_cast<PfAddCmd>(shared_from_this());
    auto resp_ptr = std::make_shared<std::string>();
    auto pika_conn = std::dynamic_pointer_cast<PikaClientConn>(GetConn());
    
    if (!pika_conn) {
      res_.SetRes(CmdRes::kErrOther, "Invalid connection");
      return;
    }
    
    callback = [self, resp_ptr, pika_conn](rocksdb::Status status) {
      if (status.ok() && self->update_) {
        self->res_.AppendInteger(1);
        AddSlotKey("h", self->key_, self->db_);
      } else if (status.ok() && !self->update_) {
        self->res_.AppendInteger(0);
      } else {
        self->res_.SetRes(CmdRes::kErrOther, status.ToString());
      }
      
      *resp_ptr = std::move(self->res_.message());
      pika_conn->WriteResp(*resp_ptr);
      pika_conn->NotifyEpoll(true);
    };
  }
  
  s_ = db_->storage()->PfAdd(key_, values_, &update_, callback);
  
  if (callback) {
    return;
  }
  
  if (s_.ok() && update_) {
    res_.AppendInteger(1);
    AddSlotKey("h", key_, db_);
  } else if (s_.ok() && !update_) {
    res_.AppendInteger(0);
  } else {
    res_.SetRes(CmdRes::kErrOther, s_.ToString());
  }
}

void PfCountCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, kCmdNamePfCount);
    return;
  }
  size_t pos = 1;
  while (pos < argv_.size()) {
    keys_.push_back(argv_[pos++]);
  }
}

void PfCountCmd::Do() {
  int64_t value_ = 0;
  rocksdb::Status s = db_->storage()->PfCount(keys_, &value_);
  if (s.ok()) {
    res_.AppendInteger(value_);
  } else {
    res_.SetRes(CmdRes::kErrOther, s.ToString());
  }
}

void PfMergeCmd::DoInitial() {
  if (!CheckArg(argv_.size())) {
    res_.SetRes(CmdRes::kWrongNum, kCmdNamePfMerge);
    return;
  }
  size_t pos = 1;
  while (pos < argv_.size()) {
    keys_.push_back(argv_[pos++]);
  }
}

void PfMergeCmd::Do() {
  storage::CommitCallback callback = nullptr;
  
  if (ShouldUseAsyncMode()) {
    auto self = std::static_pointer_cast<PfMergeCmd>(shared_from_this());
    auto resp_ptr = std::make_shared<std::string>();
    auto pika_conn = std::dynamic_pointer_cast<PikaClientConn>(GetConn());
    
    if (!pika_conn) {
      res_.SetRes(CmdRes::kErrOther, "Invalid connection");
      return;
    }
    
    callback = [self, resp_ptr, pika_conn](rocksdb::Status status) {
      if (status.ok()) {
        self->res_.SetRes(CmdRes::kOk);
        AddSlotKey("h", self->keys_[0], self->db_);
      } else {
        self->res_.SetRes(CmdRes::kErrOther, status.ToString());
      }
      
      *resp_ptr = std::move(self->res_.message());
      pika_conn->WriteResp(*resp_ptr);
      pika_conn->NotifyEpoll(true);
    };
  }
  
  s_ = db_->storage()->PfMerge(keys_, value_to_dest_, callback);
  
  if (callback) {
    return;
  }
  
  if (s_.ok()) {
    res_.SetRes(CmdRes::kOk);
    AddSlotKey("h", keys_[0], db_);
  } else {
    res_.SetRes(CmdRes::kErrOther, s_.ToString());
  }
}
void PfMergeCmd::DoBinlog() {
  PikaCmdArgsType set_args;
  //used "set" instead of "SET" to distinguish the binlog of SetCmd
  set_args.emplace_back("set");
  set_args.emplace_back(keys_[0]);
  set_args.emplace_back(value_to_dest_);
  set_cmd_->Initial(set_args,  db_name_);
  set_cmd_->SetConn(GetConn());
  set_cmd_->SetResp(resp_.lock());
  //value of this binlog might be strange, it's an string with size of 128KB
  set_cmd_->DoBinlog();
}
