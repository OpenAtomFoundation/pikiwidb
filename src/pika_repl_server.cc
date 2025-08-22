// Copyright (c) 2019-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_repl_server.h"

#include <glog/logging.h>

#include "include/pika_conf.h"
#include "include/pika_rm.h"
#include "include/pika_server.h"

using pstd::Status;

extern PikaServer* g_pika_server;
extern std::unique_ptr<PikaReplicaManager> g_pika_rm;

PikaReplServer::PikaReplServer(const std::set<std::string>& ips, int port, int cron_interval) {
  server_tp_ = std::make_unique<net::ThreadPool>(PIKA_REPL_SERVER_TP_SIZE, 100000, "PikaReplServer");
  pika_repl_server_thread_ = std::make_unique<PikaReplServerThread>(ips, port, cron_interval);
  pika_repl_server_thread_->set_thread_name("PikaReplServer");
}

PikaReplServer::~PikaReplServer() {
  LOG(INFO) << "PikaReplServer exit!!!";
}

int PikaReplServer::Start() {
  pika_repl_server_thread_->set_thread_name("PikaReplServer");
  int res = pika_repl_server_thread_->StartThread();
  if (res != net::kSuccess) {
    LOG(FATAL) << "Start Pika Repl Server Thread Error: " << res
               << (res == net::kBindError
                       ? ": bind port " + std::to_string(pika_repl_server_thread_->ListenPort()) + " conflict"
                       : ": create thread error ")
               << ", Listen on this port to handle the request sent by the Slave";
  }
  res = server_tp_->start_thread_pool();
  if (res != net::kSuccess) {
    LOG(FATAL) << "Start ThreadPool Error: " << res
               << (res == net::kCreateThreadError ? ": create thread error " : ": other error");
  }
  return res;
}

int PikaReplServer::Stop() {
  server_tp_->stop_thread_pool();
  pika_repl_server_thread_->StopThread();
  pika_repl_server_thread_->Cleanup();
  return 0;
}

pstd::Status PikaReplServer::SendSlaveBinlogChips(const std::string& ip, int port,
                                                  const std::vector<WriteTask>& tasks) {
  LOG(INFO) << "SendSlaveBinlogChips: Preparing to send " << tasks.size() << " tasks to " << ip << ":" << port;
  InnerMessage::InnerResponse response;
  BuildBinlogSyncResp(tasks, &response);

  std::string binlog_chip_pb;
  if (!response.SerializeToString(&binlog_chip_pb)) {
    return Status::Corruption("Serialized Failed");
  }

  if (binlog_chip_pb.size() > static_cast<size_t>(g_pika_conf->max_conn_rbuf_size())) {
    for (const auto& task : tasks) {
      InnerMessage::InnerResponse response;
      std::vector<WriteTask> tmp_tasks;
      tmp_tasks.push_back(task);
      BuildBinlogSyncResp(tmp_tasks, &response);
      if (!response.SerializeToString(&binlog_chip_pb)) {
        return Status::Corruption("Serialized Failed");
      }
      pstd::Status s = Write(ip, port, binlog_chip_pb);
      if (!s.ok()) {
        return s;
      }
    }
    return pstd::Status::OK();
  }
  LOG(INFO) << "SendSlaveBinlogChips: Calling Write to send " << binlog_chip_pb.size() << " bytes to " << ip << ":" << port;
  pstd::Status result = Write(ip, port, binlog_chip_pb);
  LOG(INFO) << "SendSlaveBinlogChips: Write result: " << (result.ok() ? "SUCCESS" : result.ToString());
  return result;
}

void PikaReplServer::BuildBinlogOffset(const LogOffset& offset, InnerMessage::BinlogOffset* boffset) {
  boffset->set_filenum(offset.b_offset.filenum);
  boffset->set_offset(offset.b_offset.offset);
  boffset->set_term(offset.l_offset.term);
  boffset->set_index(offset.l_offset.index);
}

void PikaReplServer::BuildBinlogSyncResp(const std::vector<WriteTask>& tasks, InnerMessage::InnerResponse* response){
  if (tasks.empty()) {
    return;
  }
  
  LOG(INFO) << "BuildBinlogSyncResp: Building response for " << tasks.size() << " tasks";
  
  response->set_type(InnerMessage::Type::kBinlogSync);
  response->set_code(InnerMessage::kOk);
  
  // Add batch magic number if there are multiple tasks
  bool is_batch = tasks.size() > 1;
  LOG(INFO) << "BuildBinlogSyncResp: is_batch=" << (is_batch ? "true" : "false") << ", batch_size=" << tasks.size();
  
  for (size_t task_idx = 0; task_idx < tasks.size(); task_idx++) {
    const auto& task = tasks[task_idx];
    InnerMessage::InnerResponse::BinlogSync* binlog_sync = response->add_binlog_sync();
    const RmNode& node = task.rm_node_;
    binlog_sync->set_session_id(node.SessionId());

    InnerMessage::Slot* db = binlog_sync->mutable_slot();
    db->set_db_name(task.rm_node_.DBName());
    /*
     * Since the slot field is written in protobuffer,
     * slot_id is set to the default value 0 for compatibility
     * with older versions, but slot_id is not used
     */
    db->set_slot_id(0);
    InnerMessage::BinlogOffset* boffset = binlog_sync->mutable_binlog_offset();
    BuildBinlogOffset(task.binlog_chip_.offset_, boffset);
    
    LOG(INFO) << "BuildBinlogSyncResp: Task " << task_idx << " offset=" << task.binlog_chip_.offset_.ToString() 
              << " binlog_size=" << task.binlog_chip_.binlog_.size();
    
    // Always add committed_id, regardless of strong consistency mode
      InnerMessage::BinlogOffset* committed_id = binlog_sync->mutable_committed_id();
      BuildBinlogOffset(task.committed_id_, committed_id);
      LOG(INFO) << "BuildBinlogSyncResp: Task " << task_idx << " committed_id=" << task.committed_id_.ToString();
    
    // For batch binlog transmission, add PIKA_BATCH_MAGIC at the beginning of the first binlog entry
    if (is_batch && binlog_sync == response->mutable_binlog_sync(0)) {
      // Prepend the magic number to indicate this is a batch
      std::string magic_binlog;
      magic_binlog.resize(sizeof(uint32_t));
      memcpy(&magic_binlog[0], &PIKA_BATCH_MAGIC, sizeof(uint32_t));
      
      // Log the magic number as hex for debugging
      LOG(INFO) << "BuildBinlogSyncResp: Adding magic number: 0x" 
                << std::hex << PIKA_BATCH_MAGIC << std::dec;
      
      // Check if binlog is empty before appending
      if (task.binlog_chip_.binlog_.empty()) {
        LOG(WARNING) << "BuildBinlogSyncResp: WARNING - Empty binlog content in batch task " << task_idx;
      }
      
      magic_binlog.append(task.binlog_chip_.binlog_);
      binlog_sync->set_binlog(magic_binlog);
      
      // Detailed logging of the binlog content
      LOG(INFO) << "BuildBinlogSyncResp: Added PIKA_BATCH_MAGIC (0x" << std::hex << PIKA_BATCH_MAGIC << std::dec 
                << ") to first binlog in batch of size " << tasks.size() 
                << ", original size=" << task.binlog_chip_.binlog_.size()
                << ", new size=" << magic_binlog.size();
      
      // Verify the magic number was correctly added by reading it back
      if (magic_binlog.size() >= sizeof(uint32_t)) {
        uint32_t verification = 0;
        memcpy(&verification, magic_binlog.data(), sizeof(uint32_t));
        LOG(INFO) << "BuildBinlogSyncResp: Verified magic number in prepared binlog: 0x" 
                  << std::hex << verification << std::dec;
      }
    } else {
      // Check if binlog is empty before setting
      if (task.binlog_chip_.binlog_.empty()) {
        LOG(WARNING) << "BuildBinlogSyncResp: WARNING - Empty binlog content in regular task " << task_idx;
      }
      
      binlog_sync->set_binlog(task.binlog_chip_.binlog_);
      LOG(INFO) << "BuildBinlogSyncResp: Regular binlog for task " << task_idx 
                << ", size=" << task.binlog_chip_.binlog_.size();
    }
  }
}

pstd::Status PikaReplServer::Write(const std::string& ip, const int port, const std::string& msg) {
  std::shared_lock l(client_conn_rwlock_);
  const std::string ip_port = pstd::IpPortString(ip, port);
  if (client_conn_map_.find(ip_port) == client_conn_map_.end()) {
    return Status::NotFound("The " + ip_port + " fd cannot be found");
  }
  int fd = client_conn_map_[ip_port];
  std::shared_ptr<net::PbConn> conn = std::dynamic_pointer_cast<net::PbConn>(pika_repl_server_thread_->get_conn(fd));
  if (!conn) {
    return Status::NotFound("The" + ip_port + " conn cannot be found");
  }

  if (conn->WriteResp(msg)) {
    conn->NotifyClose();
    return Status::Corruption("The" + ip_port + " conn, Write Resp Failed");
  }
  conn->NotifyWrite();
  return Status::OK();
}

void PikaReplServer::Schedule(net::TaskFunc func, void* arg) { server_tp_->Schedule(func, arg); }

void PikaReplServer::UpdateClientConnMap(const std::string& ip_port, int fd) {
  std::lock_guard l(client_conn_rwlock_);
  client_conn_map_[ip_port] = fd;
}

void PikaReplServer::RemoveClientConn(int fd) {
  std::lock_guard l(client_conn_rwlock_);
  auto iter = client_conn_map_.begin();
  while (iter != client_conn_map_.end()) {
    if (iter->second == fd) {
      iter = client_conn_map_.erase(iter);
      break;
    }
    iter++;
  }
}

void PikaReplServer::KillAllConns() { return pika_repl_server_thread_->KillAllConns(); }
