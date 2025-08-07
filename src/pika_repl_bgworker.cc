// Copyright (c) 2019-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_repl_bgworker.h"

#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <glog/logging.h>

#include "include/pika_cmd_table_manager.h"
#include "include/pika_conf.h"
#include "include/pika_rm.h"
#include "include/pika_server.h"
#include "pstd/include/pstd_string.h"
#include "pstd/include/pstd_defer.h"
#include "src/pstd/include/scope_record_lock.h"
#include "include/pika_conf.h"

extern PikaServer* g_pika_server;
extern std::unique_ptr<PikaReplicaManager> g_pika_rm;
extern std::unique_ptr<PikaCmdTableManager> g_pika_cmd_table_manager;

PikaReplBgWorker::PikaReplBgWorker(int queue_size) : bg_thread_(queue_size) {
  bg_thread_.set_thread_name("ReplBgWorker");
  net::RedisParserSettings settings;
  settings.DealMessage = &(PikaReplBgWorker::HandleWriteBinlog);
  redis_parser_.RedisParserInit(REDIS_PARSER_REQUEST, settings);
  redis_parser_.data = this;
  db_name_ = g_pika_conf->default_db();
}

int PikaReplBgWorker::StartThread() { return bg_thread_.StartThread(); }

int PikaReplBgWorker::StopThread() { return bg_thread_.StopThread(); }

void PikaReplBgWorker::Schedule(net::TaskFunc func, void* arg) { bg_thread_.Schedule(func, arg); }

void PikaReplBgWorker::Schedule(net::TaskFunc func, void* arg, std::function<void()>& call_back) {
  bg_thread_.Schedule(func, arg, call_back);
}

void PikaReplBgWorker::ParseBinlogOffset(const InnerMessage::BinlogOffset& pb_offset, LogOffset* offset) {
  offset->b_offset.filenum = pb_offset.filenum();
  offset->b_offset.offset = pb_offset.offset();
  offset->l_offset.term = pb_offset.term();
  offset->l_offset.index = pb_offset.index();
}

void PikaReplBgWorker::HandleBGWorkerWriteBinlog(void* arg) {
  auto task_arg = static_cast<ReplClientWriteBinlogTaskArg*>(arg);
  const std::shared_ptr<InnerMessage::InnerResponse> res = task_arg->res;
  std::shared_ptr<net::PbConn> conn = task_arg->conn;
  auto index = static_cast<std::vector<int>*>(task_arg->res_private_data);
  PikaReplBgWorker* worker = task_arg->worker;
  worker->ip_port_ = conn->ip_port();

  LOG(INFO) << "HandleBGWorkerWriteBinlog: Received binlog from master " << worker->ip_port_ 
            << ", index size: " << index->size();

  DEFER { 
    delete index;
    delete task_arg;
  };

  std::string db_name;

  LogOffset pb_begin;
  LogOffset pb_end;
  bool only_keepalive = false;

  // find the first not keepalive binlogsync
  for (size_t i = 0; i < index->size(); ++i) {
    const InnerMessage::InnerResponse::BinlogSync& binlog_res = res->binlog_sync((*index)[i]);
    if (i == 0) {
      db_name = binlog_res.slot().db_name();
    }
    if (!binlog_res.binlog().empty()) {
      ParseBinlogOffset(binlog_res.binlog_offset(), &pb_begin);
      break;
    }
  }

  // find the last not keepalive binlogsync
  for (int i = static_cast<int>(index->size() - 1); i >= 0; i--) {
    const InnerMessage::InnerResponse::BinlogSync& binlog_res = res->binlog_sync((*index)[i]);
    if (!binlog_res.binlog().empty()) {
      ParseBinlogOffset(binlog_res.binlog_offset(), &pb_end);
      break;
    }
  }

  if (pb_begin == LogOffset()) {
    only_keepalive = true;
  }

  LogOffset ack_start;
  LogOffset ack_end;

  if (only_keepalive) {
    ack_start = LogOffset();
    ack_end = LogOffset();
  } else {
    ack_start = pb_begin;
    ack_end = pb_end;
  }

  // because DispatchBinlogRes() have been order them.
  worker->db_name_ = db_name;

  std::shared_ptr<SyncMasterDB> db =
      g_pika_rm->GetSyncMasterDBByName(DBInfo(db_name));
  if (!db) {
    LOG(WARNING) << "DB " << db_name << " Not Found";
    return;
  }

  std::shared_ptr<SyncSlaveDB> slave_db =
      g_pika_rm->GetSyncSlaveDBByName(DBInfo(db_name));
  if (!slave_db) {
    LOG(WARNING) << "Slave DB " << db_name << " Not Found";
    return;
  }

  int processed_count = 0;
  for (int i : *index) {
    const InnerMessage::InnerResponse::BinlogSync& binlog_res = res->binlog_sync(i);
    // if pika are not current a slave or DB not in
    // BinlogSync state, we drop remain write binlog task
    if (((g_pika_server->role() & PIKA_ROLE_SLAVE) == 0) ||
        ((slave_db->State() != ReplState::kConnected) && (slave_db->State() != ReplState::kWaitDBSync))) {
      return;
    }

    if (slave_db->MasterSessionId() != binlog_res.session_id()) {
      LOG(WARNING) << "Check SessionId Mismatch: " << slave_db->MasterIp() << ":"
                   << slave_db->MasterPort() << ", " << slave_db->SyncDBInfo().ToString()
                   << " expected_session: " << binlog_res.session_id()
                   << ", actual_session:" << slave_db->MasterSessionId();
      LOG(WARNING) << "Check Session failed " << binlog_res.slot().db_name();
      slave_db->SetReplState(ReplState::kTryConnect);
      return;
    }
    if(db->GetISConsistency()){
      const InnerMessage::BinlogOffset& committed_id = binlog_res.committed_id();
      LogOffset master_committed_id(BinlogOffset(committed_id.filenum(),committed_id.offset()),LogicOffset(committed_id.term(),committed_id.index()));
      LOG(INFO) << "Processing committed_id from master: " << master_committed_id.ToString();
      Status s= db->CommitAppLog(master_committed_id);
      if(!s.ok()){
        return;
      }
    }
    // empty binlog treated as keepalive packet
    if (binlog_res.binlog().empty()) {
      LOG(INFO) << "Received keepalive packet from master";
      continue;
    }
    
    std::vector<std::string> individual_binlogs;
    const std::string& received_binlog = binlog_res.binlog();
    const uint32_t BATCH_MAGIC = htonl(PIKA_BATCH_MAGIC);

    if (received_binlog.size() >= sizeof(BATCH_MAGIC) &&
        *reinterpret_cast<const uint32_t*>(received_binlog.data()) == BATCH_MAGIC) {
      // This is a batched binlog
      LOG(INFO) << "Received batched binlog from master, size: " << received_binlog.size();
      const char* ptr = received_binlog.data() + sizeof(BATCH_MAGIC);
      const char* end = received_binlog.data() + received_binlog.size();
      while (ptr < end) {
        if (ptr + sizeof(uint32_t) > end) {
          LOG(WARNING) << "Batched binlog format error: incomplete length field.";
          slave_db->SetReplState(ReplState::kTryConnect);
          return;
        }
        uint32_t item_len = ntohl(*reinterpret_cast<const uint32_t*>(ptr));
        ptr += sizeof(uint32_t);
        if (ptr + item_len > end) {
          LOG(WARNING) << "Batched binlog format error: incomplete binlog data.";
          slave_db->SetReplState(ReplState::kTryConnect);
          return;
        }
        individual_binlogs.emplace_back(ptr, item_len);
        ptr += item_len;
      }
      LOG(INFO) << "Received " << individual_binlogs.size() << " individual binlogs in batch for db " << db_name;
    } else {
      // This is a single binlog
      LOG(INFO) << "Received single binlog from master, size: " << received_binlog.size();
      individual_binlogs.push_back(received_binlog);
    }

    for (const auto& binlog_str : individual_binlogs) {
      if (!PikaBinlogTransverter::BinlogItemWithoutContentDecode(TypeFirst, binlog_str, &worker->binlog_item_)) {
        LOG(WARNING) << "Binlog item decode failed";
        slave_db->SetReplState(ReplState::kTryConnect);
        return;
      }
      const char* redis_parser_start = binlog_str.data() + BINLOG_ENCODE_LEN;
      int redis_parser_len = static_cast<int>(binlog_str.size()) - BINLOG_ENCODE_LEN;
      int processed_len = 0;
      net::RedisParserStatus ret =
          worker->redis_parser_.ProcessInputBuffer(redis_parser_start, redis_parser_len, &processed_len);
      if (ret != net::kRedisParserDone) {
        LOG(WARNING) << "Redis parser failed";
        slave_db->SetReplState(ReplState::kTryConnect);
        return;
      }
      processed_count++;
    }
    LOG(INFO) << "Processed " << processed_count << " binlog entries for db " << db_name;
  }
  
  LOG(INFO) << "Successfully processed " << processed_count << " binlog entries";

  if (only_keepalive) {
    ack_end = LogOffset();
    LOG(INFO) << "Sending keepalive ACK to master";
  } else {
    LogOffset productor_status;
    // Reply Ack to master immediately
    std::shared_ptr<Binlog> logger = db->Logger();
    logger->GetProducerStatus(&productor_status.b_offset.filenum, &productor_status.b_offset.offset,
                              &productor_status.l_offset.term, &productor_status.l_offset.index);
    ack_end = productor_status;
    ack_end.l_offset.term = pb_end.l_offset.term;
    
    //Force flush to ensure data persistence
    Status s = logger->Sync();
    if (!s.ok()) {
      LOG(WARNING) << "Failed to sync binlog to disk: " << s.ToString();
      return;
    }
    LOG(INFO) << "Synced binlog to disk, sending ACK to master from " 
              << ack_start.ToString() << " to " << ack_end.ToString();
  }

  LOG(INFO) << "Sending ACK for db " << db_name << " from " << ack_start.ToString() << " to " << ack_end.ToString();
  g_pika_rm->SendBinlogSyncAckRequest(db_name, ack_start, ack_end);
}

int PikaReplBgWorker::HandleWriteBinlog(net::RedisParser* parser, const net::RedisCmdArgsType& argv) {
  std::string opt = argv[0];
  auto worker = static_cast<PikaReplBgWorker*>(parser->data);
  // Monitor related
  std::string monitor_message;
  if (g_pika_server->HasMonitorClients()) {
    std::string db_name = worker->db_name_.substr(2);
    std::string monitor_message =
        std::to_string(static_cast<double>(pstd::NowMicros()) / 1000000) + " [" + db_name + " " + worker->ip_port_ + "]";
    for (const auto& item : argv) {
      monitor_message += " " + pstd::ToRead(item);
    }
    g_pika_server->AddMonitorMessage(monitor_message);
  }

  std::shared_ptr<Cmd> c_ptr = g_pika_cmd_table_manager->GetCmd(pstd::StringToLower(opt));
  if (!c_ptr) {
    LOG(WARNING) << "Command " << opt << " not in the command db";
    return -1;
  }
  // Initial
  c_ptr->Initial(argv, worker->db_name_);
  if (!c_ptr->res().ok()) {
    LOG(WARNING) << "Fail to initial command from binlog: " << opt;
    return -1;
  }

  g_pika_server->UpdateQueryNumAndExecCountDB(worker->db_name_, opt, c_ptr->is_write());

  std::shared_ptr<SyncMasterDB> db =
      g_pika_rm->GetSyncMasterDBByName(DBInfo(worker->db_name_));
  if (!db) {
    LOG(WARNING) << worker->db_name_ << "Not found.";
    return -1;
  }
  if(db->GetISConsistency()){
    db->AppendSlaveEntries(c_ptr, worker->binlog_item_);
  }else{
    db->ConsensusProcessLeaderLog(c_ptr, worker->binlog_item_);
  }
  return 0;
}

void PikaReplBgWorker::HandleBGWorkerWriteDB(void* arg) {
  std::unique_ptr<std::shared_ptr<Cmd>> cmd_ptr_ptr(static_cast<std::shared_ptr<Cmd>*>(arg));
  const std::shared_ptr<Cmd> c_ptr = *cmd_ptr_ptr;
  WriteDBInSyncWay(c_ptr);
}

void PikaReplBgWorker::WriteDBInSyncWay(const std::shared_ptr<Cmd>& c_ptr) {
  const PikaCmdArgsType& argv = c_ptr->argv();

  uint64_t start_us = 0;
  if (g_pika_conf->slowlog_slower_than() >= 0) {
    start_us = pstd::NowMicros();
  }
  // Add read lock for no suspend command
  pstd::lock::MultiRecordLock record_lock(c_ptr->GetDB()->LockMgr());
  record_lock.Lock(c_ptr->current_key());
  if (!c_ptr->IsSuspend()) {
    c_ptr->GetDB()->DBLockShared();
  }
  if (c_ptr->IsNeedCacheDo()
      && PIKA_CACHE_NONE != g_pika_conf->cache_mode()
      && c_ptr->GetDB()->cache()->CacheStatus() == PIKA_CACHE_STATUS_OK) {
    if (c_ptr->is_write()) {
      c_ptr->DoThroughDB();
      if (c_ptr->IsNeedUpdateCache()) {
        c_ptr->DoUpdateCache();
      }
    } else {
      LOG(WARNING) << "It is impossbile to reach here";
    }
  } else {
    c_ptr->Do();
  }
  if (!c_ptr->IsSuspend()) {
    c_ptr->GetDB()->DBUnlockShared();
  }

  if (c_ptr->res().ok()
      && c_ptr->is_write()
      && c_ptr->name() != kCmdNameFlushdb
      && c_ptr->name() != kCmdNameFlushall
      && c_ptr->name() != kCmdNameExec) {
    auto table_keys = c_ptr->current_key();
    for (auto& key : table_keys) {
      key = c_ptr->db_name().append(key);
    }
    auto dispatcher = dynamic_cast<net::DispatchThread*>(g_pika_server->pika_dispatch_thread()->server_thread());
    auto involved_conns = dispatcher->GetInvolvedTxn(table_keys);
    for (auto& conn : involved_conns) {
      auto c = std::dynamic_pointer_cast<PikaClientConn>(conn);
      c->SetTxnWatchFailState(true);
    }
  }

  record_lock.Unlock(c_ptr->current_key());
  if (g_pika_conf->slowlog_slower_than() >= 0) {
    auto start_time = static_cast<int32_t>(start_us / 1000000);
    auto duration = static_cast<int64_t>(pstd::NowMicros() - start_us);
    if (duration > g_pika_conf->slowlog_slower_than()) {
      g_pika_server->SlowlogPushEntry(argv, start_time, duration);
      if (g_pika_conf->slowlog_write_errorlog()) {
        LOG(INFO) << "command: " << argv[0] << ", start_time(s): " << start_time << ", duration(us): " << duration;
      }
    }
  }
}
