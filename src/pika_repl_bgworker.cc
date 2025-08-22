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
            << ", index size: " << index->size() 
            << ", binlog_sync_size: " << res->binlog_sync_size();

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
      LOG(WARNING) << "Check Session failed " << binlog_res.slot().db_name() << ", setting to kError (no auto-reconnect)";
      slave_db->SetReplState(ReplState::kError);
      return;
    }
    if(db->GetISConsistency()){
      const InnerMessage::BinlogOffset& committed_id = binlog_res.committed_id();
      LogOffset master_committed_id(BinlogOffset(committed_id.filenum(),committed_id.offset()),LogicOffset(committed_id.term(),committed_id.index()));
      Status s= db->CommitAppLog(master_committed_id);
      if(!s.ok()){
        return;
      }
    }
    // empty binlog treated as keepalive packet
    if (binlog_res.binlog().empty()) {
      LOG(INFO) << "HandleBGWorkerWriteBinlog: Skipping empty binlog (keepalive packet) at index " << i;
      continue;
    }
    // Handle binlog data with batch magic number detection
    const std::string& received_binlog = binlog_res.binlog();
    std::string binlog_str = received_binlog;

    // Check if this is the first binlog entry in a batch (contains PIKA_BATCH_MAGIC)
    bool has_batch_magic = false;
    if (i == (*index)[0] && received_binlog.size() >= sizeof(uint32_t)) {
      uint32_t magic_num = 0;
      memcpy(&magic_num, received_binlog.data(), sizeof(uint32_t));
      if (magic_num == PIKA_BATCH_MAGIC) {
        has_batch_magic = true;
        // Remove the magic number from the binlog data
        binlog_str = received_binlog.substr(sizeof(uint32_t));
        LOG(INFO) << "HandleBGWorkerWriteBinlog: Detected PIKA_BATCH_MAGIC in binlog entry " << i
                  << ", processing as batch, original size: " << received_binlog.size()
                  << ", new size after removing magic: " << binlog_str.size();
      }
    }

    // Validate binlog data
    if (binlog_str.empty()) {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: Empty binlog data after processing";
      continue;
    }

    if (binlog_str.size() < BINLOG_ENCODE_LEN) {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: Binlog data too small (" << binlog_str.size()
                   << " bytes), minimum required: " << BINLOG_ENCODE_LEN;
      continue;
    }

    // Decode the binlog item
    if (!PikaBinlogTransverter::BinlogItemWithoutContentDecode(TypeFirst, binlog_str, &worker->binlog_item_)) {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: Binlog item decode failed for entry " << i;
      slave_db->SetReplState(ReplState::kTryConnect);
      return;
    }

    // Extract Redis command data
    const char* redis_parser_start = binlog_str.data() + BINLOG_ENCODE_LEN;
    int redis_parser_len = static_cast<int>(binlog_str.size()) - BINLOG_ENCODE_LEN;

    if (redis_parser_len <= 0) {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: No Redis command data after binlog header for entry " << i;
      continue;
    }

    // Validate Redis protocol data
    if (redis_parser_start[0] != '*' && redis_parser_start[0] != '+' &&
        redis_parser_start[0] != '-' && redis_parser_start[0] != ':' &&
        redis_parser_start[0] != '$') {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: Invalid Redis protocol start character: "
                   << static_cast<int>(redis_parser_start[0]) << " for binlog entry " << i;
      continue;
    }

    // Create a new parser instance for each binlog entry to ensure clean state
    net::RedisParser temp_parser;
    net::RedisParserSettings settings;
    settings.DealMessage = &(PikaReplBgWorker::HandleWriteBinlog);
    temp_parser.RedisParserInit(REDIS_PARSER_REQUEST, settings);
    temp_parser.data = worker;

    int processed_len = 0;
    net::RedisParserStatus ret =
        temp_parser.ProcessInputBuffer(redis_parser_start, redis_parser_len, &processed_len);

    if (ret != net::kRedisParserDone) {
      LOG(WARNING) << "HandleBGWorkerWriteBinlog: Redis parser failed for entry " << i
                   << ", status: " << static_cast<int>(ret)
                   << ", processed: " << processed_len << " of " << redis_parser_len << " bytes"
                   << (has_batch_magic ? " (batch mode)" : "");
      // For batch processing, continue with other entries instead of failing completely
      if (has_batch_magic && (*index).size() > 1) {
        LOG(INFO) << "HandleBGWorkerWriteBinlog: Continuing with next entry after parser error in batch";
        continue;
      } else {
        slave_db->SetReplState(ReplState::kTryConnect);
        return;
      }
    }

    // Successfully processed this binlog entry
    processed_count++;
    LOG(INFO) << "HandleBGWorkerWriteBinlog: Successfully processed binlog entry " << i
              << (has_batch_magic ? " (batch mode)" : "");

    db = g_pika_rm->GetSyncMasterDBByName(DBInfo(worker->db_name_));
    if (!db) {
       LOG(WARNING) << "DB " << worker->db_name_ << " Not Found";
       return;
     }
  }

  if (only_keepalive) {
    ack_end = LogOffset();
    LOG(INFO) << "HandleBGWorkerWriteBinlog: Only keepalive packets received, sending empty ACK";
    // Log more detailed debug information about the keepalive situation
    LOG(WARNING) << "HandleBGWorkerWriteBinlog: No binlog entries were processed. This may indicate an issue with binlog transmission or format.";
    LOG(INFO) << "HandleBGWorkerWriteBinlog: Total binlog entries received: " << index->size() 
              << ", processed: " << processed_count;
  } else {
    ack_end = pb_end;
    LOG(INFO) << "HandleBGWorkerWriteBinlog: Processed " << processed_count << " binlog entries, sending ACK"
              << " for " << db_name 
              << ", ack_start: " << ack_start.ToString() 
              << ", ack_end: " << ack_end.ToString();
  }

  // Get current slave DB state before sending ACK
  ReplState current_state = slave_db->State();
  LOG(INFO) << "HandleBGWorkerWriteBinlog: Current slave state before sending ACK: " 
            << static_cast<int>(current_state) 
            << " (" << (current_state == ReplState::kConnected ? "Connected" : 
                       (current_state == ReplState::kWaitDBSync ? "WaitDBSync" : "Other")) << ")";

  // Send the ACK with detailed logging
  Status ack_status = g_pika_rm->SendBinlogSyncAckRequest(db_name, ack_start, ack_end);
  LOG(INFO) << "HandleBGWorkerWriteBinlog: ACK send result: " 
            << (ack_status.ok() ? "Success" : "Failed: " + ack_status.ToString());
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
  if (db->GetISConsistency()) {
    Status s = db->AppendSlaveEntries(c_ptr, worker->binlog_item_);
    if (s.IsInvalidArgument()) {
      // This happens when the slave receives a binlog with an old index,
      // which is likely caused by a lost ACK.
      // We should resend our latest ACK to help the master correct its state.
      LogOffset last_offset;
      db->Logger()->GetProducerStatus(&last_offset.b_offset.filenum, &last_offset.b_offset.offset, &last_offset.l_offset.term, &last_offset.l_offset.index);
      LOG(INFO) << "Resending ACK for " << worker->db_name_ << " because of expired log. ACK offset: " << last_offset.ToString();
      g_pika_rm->SendBinlogSyncAckRequest(worker->db_name_, last_offset, last_offset);
    }
  } else {
    db->ConsensusProcessLeaderLog(c_ptr, worker->binlog_item_);
  }

  // log details of the binlog item
  LOG(INFO) << "Current binlog item from master - term: " << worker->binlog_item_.term_id()
            << " logic_id: " << worker->binlog_item_.logic_id()
            << " filenum: " << worker->binlog_item_.filenum()
            << " offset: " << worker->binlog_item_.offset();
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
