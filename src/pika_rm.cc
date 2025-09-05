// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/pika_rm.h"

#include <arpa/inet.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <utility>
#include <chrono>
#include <unordered_set>

#include "net/include/net_cli.h"

#include "include/pika_conf.h"
#include "include/pika_server.h"

#include "include/pika_admin.h"
#include "include/pika_command.h"

using pstd::Status;

extern std::unique_ptr<PikaReplicaManager> g_pika_rm;
extern PikaServer* g_pika_server;
extern std::unique_ptr<PikaConf> g_pika_conf;

/* SyncDB */

SyncDB::SyncDB(const std::string& db_name)
    : db_info_(db_name) {}

std::string SyncDB::DBName() {
  return db_info_.db_name_;
}

/* SyncMasterDB*/

SyncMasterDB::SyncMasterDB(const std::string& db_name)
    : SyncDB(db_name),  coordinator_(db_name) {
  command_collector_ = std::make_shared<PikaCommandCollector>(&coordinator_, 5);
}

int SyncMasterDB::GetNumberOfSlaveNode() { return coordinator_.SyncPros().SlaveSize(); }

bool SyncMasterDB::CheckSlaveNodeExist(const std::string& ip, int port) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  return static_cast<bool>(slave_ptr);
}

Status SyncMasterDB::GetSlaveNodeSession(const std::string& ip, int port, int32_t* session) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("slave " + ip + ":" + std::to_string(port) + " not found");
  }

  slave_ptr->Lock();
  *session = slave_ptr->SessionId();
  slave_ptr->Unlock();

  return Status::OK();
}

Status SyncMasterDB::AddSlaveNode(const std::string& ip, int port, int session_id) {
  Status s = coordinator_.AddSlaveNode(ip, port, session_id);
  if (!s.ok()) {
    LOG(WARNING) << "Add Slave Node Failed, db: " << SyncDBInfo().ToString() << ", ip_port: " << ip << ":"
                 << port;
    return s;
  }
  LOG(INFO) << "Add Slave Node, db: " << SyncDBInfo().ToString() << ", ip_port: " << ip << ":" << port;
  return Status::OK();
}

Status SyncMasterDB::RemoveSlaveNode(const std::string& ip, int port) {
  Status s = coordinator_.RemoveSlaveNode(ip, port);
  if (!s.ok()) {
    LOG(WARNING) << "Remove Slave Node Failed, db: " << SyncDBInfo().ToString() << ", ip_port: " << ip
                 << ":" << port;
    return s;
  }
  LOG(INFO) << "Remove Slave Node, DB: " << SyncDBInfo().ToString() << ", ip_port: " << ip << ":" << port;
  return Status::OK();
}

Status SyncMasterDB::ActivateSlaveBinlogSync(const std::string& ip, int port, const LogOffset& offset) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  {
    std::lock_guard l(slave_ptr->slave_mu);
    slave_ptr->slave_state = kSlaveBinlogSync;
    slave_ptr->sent_offset = offset;
    slave_ptr->acked_offset = offset;
    // read binlog file from file
    Status s = slave_ptr->InitBinlogFileReader(Logger(), offset.b_offset);
    if (!s.ok()) {
      return Status::Corruption("Init binlog file reader failed" + s.ToString());
    }
    //Since we init a new reader, we should drop items in write queue and reset sync_window.
    //Or the sent_offset and acked_offset will not match
    g_pika_rm->DropItemInOneWriteQueue(ip, port, slave_ptr->DBName());
    slave_ptr->sync_win.Reset();
    slave_ptr->b_state = kReadFromFile;
  }

  Status s = SyncBinlogToWq(ip, port);
  if (!s.ok()) {
    return s;
  }
  return Status::OK();
}

Status SyncMasterDB::SyncBinlogToWq(const std::string& ip, int port) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }
  Status s;
  slave_ptr->Lock();
  if (coordinator_.GetISConsistency()) {
    s = coordinator_.SendBinlog(slave_ptr, slave_ptr->DBName());
  }else{
    s = ReadBinlogFileToWq(slave_ptr);
  }
  slave_ptr->Unlock();
  if (!s.ok()) {
    return s;
  }
  return Status::OK();
}

Status SyncMasterDB::ActivateSlaveDbSync(const std::string& ip, int port) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  slave_ptr->Lock();
  slave_ptr->slave_state = kSlaveDbSync;
  // invoke db sync
  slave_ptr->Unlock();

  return Status::OK();
}

Status SyncMasterDB::ReadBinlogFileToWq(const std::shared_ptr<SlaveNode>& slave_ptr) {
  int cnt = slave_ptr->sync_win.Remaining();
  std::shared_ptr<PikaBinlogReader> reader = slave_ptr->binlog_reader;
  if (!reader) {
    return Status::OK();
  }
  std::vector<WriteTask> tasks;
  for (int i = 0; i < cnt; ++i) {
    std::string msg;
    uint32_t filenum;
    uint64_t offset;
    if (slave_ptr->sync_win.GetTotalBinlogSize() > PIKA_MAX_CONN_RBUF_HB * 2) {
      LOG(INFO) << slave_ptr->ToString()
                << " total binlog size in sync window is :" << slave_ptr->sync_win.GetTotalBinlogSize();
      break;
    }
    Status s = reader->Get(&msg, &filenum, &offset);
    if (s.IsEndFile()) {
      break;
    } else if (s.IsCorruption() || s.IsIOError()) {
      LOG(WARNING) << SyncDBInfo().ToString() << " Read Binlog error : " << s.ToString();
      return s;
    }
    BinlogItem item;
    if (!PikaBinlogTransverter::BinlogItemWithoutContentDecode(TypeFirst, msg, &item)) {
      LOG(WARNING) << "Binlog item decode failed";
      return Status::Corruption("Binlog item decode failed");
    }
    BinlogOffset sent_b_offset = BinlogOffset(filenum, offset);
    LogicOffset sent_l_offset = LogicOffset(item.term_id(), item.logic_id());
    LogOffset sent_offset(sent_b_offset, sent_l_offset);

    slave_ptr->sync_win.Push(SyncWinItem(sent_offset, msg.size()));
    slave_ptr->SetLastSendTime(pstd::NowMicros());
    RmNode rm_node(slave_ptr->Ip(), slave_ptr->Port(), slave_ptr->DBName(), slave_ptr->SessionId());
    WriteTask task(rm_node, BinlogChip(sent_offset, msg), slave_ptr->sent_offset);
    tasks.push_back(task);
    slave_ptr->sent_offset = sent_offset;
  }

  if (!tasks.empty()) {
    g_pika_rm->ProduceWriteQueue(slave_ptr->Ip(), slave_ptr->Port(), db_info_.db_name_, tasks);
  }
  return Status::OK();
}

Status SyncMasterDB::ConsensusUpdateSlave(const std::string& ip, int port, const LogOffset& start, const LogOffset& end) {
  Status s = coordinator_.UpdateSlave(ip, port, start, end);
  if (!s.ok()) {
    LOG(WARNING) << SyncDBInfo().ToString() << s.ToString();
    return s;
  }
  return Status::OK();
}

Status SyncMasterDB::GetSlaveSyncBinlogInfo(const std::string& ip, int port, BinlogOffset* sent_offset,
                                            BinlogOffset* acked_offset) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  slave_ptr->Lock();
  *sent_offset = slave_ptr->sent_offset.b_offset;
  *acked_offset = slave_ptr->acked_offset.b_offset;
  slave_ptr->Unlock();

  return Status::OK();
}

Status SyncMasterDB::GetSlaveState(const std::string& ip, int port, SlaveState* const slave_state) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  slave_ptr->Lock();
  *slave_state = slave_ptr->slave_state;
  slave_ptr->Unlock();

  return Status::OK();
}

Status SyncMasterDB::WakeUpSlaveBinlogSync() {
    std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();
    std::vector<std::shared_ptr<SlaveNode>> to_del;
    for (auto& slave_iter : slaves) {
        std::shared_ptr<SlaveNode> slave_ptr = slave_iter.second;
        std::lock_guard l(slave_ptr->slave_mu);
        if (slave_ptr->sent_offset == slave_ptr->acked_offset) {
          Status s;
          if (coordinator_.GetISConsistency()) {
            if(slave_ptr->slave_state == SlaveState::kSlaveBinlogSync||slave_ptr->slave_state == SlaveState::KCandidate){
              s = coordinator_.SendBinlog(slave_ptr, db_info_.db_name_);
            }
          } else {
            s = ReadBinlogFileToWq(slave_ptr);
          }
          if (!s.ok()) {
            to_del.push_back(slave_ptr);
            // LOG(WARNING) << "WakeUpSlaveBinlogSync failed, marking for deletion: "
            //                  << slave_ptr->ToStringStatus() << " - " << s.ToString();
        }
      }
  }

    for (const auto& to_del_slave : to_del) {
        RemoveSlaveNode(to_del_slave->Ip(), to_del_slave->Port());
        LOG(INFO) << "Removed slave: " << to_del_slave->ToStringStatus();
    }

    return Status::OK();
}


Status SyncMasterDB::SetLastRecvTime(const std::string& ip, int port, uint64_t time) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  slave_ptr->Lock();
  slave_ptr->SetLastRecvTime(time);
  slave_ptr->Unlock();

  return Status::OK();
}

Status SyncMasterDB::GetSafetyPurgeBinlog(std::string* safety_purge) {
  BinlogOffset boffset;
  Status s = Logger()->GetProducerStatus(&(boffset.filenum), &(boffset.offset));
  if (!s.ok()) {
    return s;
  }
  bool success = false;
  uint32_t purge_max = boffset.filenum;
  if (purge_max >= 10) {
    success = true;
    purge_max -= 10;
    std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();
    for (const auto& slave_iter : slaves) {
      std::shared_ptr<SlaveNode> slave_ptr = slave_iter.second;
      std::lock_guard l(slave_ptr->slave_mu);
      if (slave_ptr->slave_state == SlaveState::kSlaveBinlogSync && slave_ptr->acked_offset.b_offset.filenum > 0) {
        purge_max = std::min(slave_ptr->acked_offset.b_offset.filenum - 1, purge_max);
      } else {
        success = false;
        break;
      }
    }
  }
  *safety_purge = (success ? kBinlogPrefix + std::to_string(static_cast<int32_t>(purge_max)) : "none");
  return Status::OK();
}

bool SyncMasterDB::BinlogCloudPurge(uint32_t index) {
  BinlogOffset boffset;
  Status s = Logger()->GetProducerStatus(&(boffset.filenum), &(boffset.offset));
  if (!s.ok()) {
    return false;
  }
  if (index > (boffset.filenum - 10)) {  // remain some more
    return false;
  } else {
    std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();
    for (const auto& slave_iter : slaves) {
      std::shared_ptr<SlaveNode> slave_ptr = slave_iter.second;
      std::lock_guard l(slave_ptr->slave_mu);
      if (slave_ptr->slave_state == SlaveState::kSlaveDbSync) {
        return false;
      } else if (slave_ptr->slave_state == SlaveState::kSlaveBinlogSync) {
        if (index >= slave_ptr->acked_offset.b_offset.filenum) {
          return false;
        }
      }
    }
  }
  return true;
}

Status SyncMasterDB::CheckSyncTimeout(uint64_t now) {
  std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();

  std::vector<Node> to_del;
  for (auto& slave_iter : slaves) {
    std::shared_ptr<SlaveNode> slave_ptr = slave_iter.second;
    std::lock_guard l(slave_ptr->slave_mu);
    if (slave_ptr->LastRecvTime() + kRecvKeepAliveTimeout < now) {
      to_del.emplace_back(slave_ptr->Ip(), slave_ptr->Port());
    } else if (slave_ptr->LastSendTime() + kSendKeepAliveTimeout < now &&
               slave_ptr->sent_offset == slave_ptr->acked_offset) {
      std::vector<WriteTask> task;
      RmNode rm_node(slave_ptr->Ip(), slave_ptr->Port(), slave_ptr->DBName(), slave_ptr->SessionId());
      WriteTask empty_task(rm_node, BinlogChip(LogOffset(), ""), LogOffset());
      if(GetISConsistency()){
        empty_task = WriteTask(rm_node, BinlogChip(LogOffset(), ""), LogOffset(),GetCommittedId());
      }
      task.push_back(empty_task);
      Status s = g_pika_rm->SendSlaveBinlogChipsRequest(slave_ptr->Ip(), slave_ptr->Port(), task);
      slave_ptr->SetLastSendTime(now);
      if (!s.ok()) {
        LOG(INFO) << "Send ping failed: " << s.ToString();
        return Status::Corruption("Send ping failed: " + slave_ptr->Ip() + ":" + std::to_string(slave_ptr->Port()));
      }
    }
  }

  for (auto& node : to_del) {
    coordinator_.SyncPros().RemoveSlaveNode(node.Ip(), node.Port());
    g_pika_rm->DropItemInOneWriteQueue(node.Ip(), node.Port(), DBName());
    LOG(WARNING) << SyncDBInfo().ToString() << " Master del Recv Timeout slave success " << node.ToString();
  }
  return Status::OK();
}

std::string SyncMasterDB::ToStringStatus() {
  std::stringstream tmp_stream;
  tmp_stream << " Current Master Session: " << session_id_ << "\r\n";
  tmp_stream << "  Consensus: "
             << "\r\n"
             << coordinator_.ToStringStatus();
  std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();
  int i = 0;
  for (const auto& slave_iter : slaves) {
    std::shared_ptr<SlaveNode> slave_ptr = slave_iter.second;
    std::lock_guard l(slave_ptr->slave_mu);
    tmp_stream << "  slave[" << i << "]: " << slave_ptr->ToString() << "\r\n" << slave_ptr->ToStringStatus();
    i++;
  }
  return tmp_stream.str();
}

int32_t SyncMasterDB::GenSessionId() {
  std::lock_guard ml(session_mu_);
  return session_id_++;
}

bool SyncMasterDB::CheckSessionId(const std::string& ip, int port, const std::string& db_name,
                                  int session_id) {
  std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    LOG(WARNING) << "Check SessionId Get Slave Node Error: " << ip << ":" << port << "," << db_name;
    return false;
  }

  std::lock_guard l(slave_ptr->slave_mu);
  if (session_id != slave_ptr->SessionId()) {
    LOG(WARNING) << "Check SessionId Mismatch: " << ip << ":" << port << ", " << db_name << "_"
                 << " expected_session: " << session_id << ", actual_session:" << slave_ptr->SessionId();
    return false;
  }
  return true;
}

bool SyncMasterDB::checkFinished(const LogOffset& offset){
  return coordinator_.checkFinished(offset);
}
void SyncMasterDB::SetConsistency(bool is_consistenct){
  coordinator_.SetConsistency(is_consistenct);
}
bool SyncMasterDB::GetISConsistency(){
  return coordinator_.GetISConsistency();
}
void SyncMasterDB::SetPreparedId(const LogOffset& offset){
  coordinator_.SetPreparedId(offset);
}
void SyncMasterDB::SetCommittedId(const LogOffset& offset){
  coordinator_.SetCommittedId(offset);
}
LogOffset SyncMasterDB::GetPreparedId(){
  return coordinator_.GetPreparedId();
}
LogOffset SyncMasterDB::GetCommittedId(){
  return coordinator_.GetCommittedId();
}

Status SyncMasterDB::AppendSlaveEntries(const std::shared_ptr<Cmd>& cmd_ptr, const BinlogItem& attribute) {
  return coordinator_.AppendSlaveEntries(cmd_ptr, attribute);
}

Status SyncMasterDB::ProcessCoordination(){
  return coordinator_.ProcessCoordination();
}
Status SyncMasterDB::UpdateCommittedID(){
  Status s = coordinator_.UpdateCommittedID();
  if (s.ok()) {
    // Notify RocksDB thread of new CommittedID
    LogOffset committed_id = coordinator_.GetCommittedId();
    if (committed_id.IsValid()) {
      extern std::unique_ptr<PikaReplicaManager> g_pika_rm;
      if (g_pika_rm) {
        g_pika_rm->NotifyCommittedID(committed_id);
      }
    }
  } else {
    LOG(WARNING) << "UpdateCommittedID failed: " << s.ToString();
  }
  return s;
}
Status SyncMasterDB::Truncate(const LogOffset& offset){
  return coordinator_.Truncate(offset);
}
Status SyncMasterDB::CommitAppLog(const LogOffset& master_committed_id){
  return coordinator_.CommitAppLog(master_committed_id);
}
Status SyncMasterDB::AppendCandidateBinlog(const std::string& ip, int port, const LogOffset& offset) {
   std::shared_ptr<SlaveNode> slave_ptr = GetSlaveNode(ip, port);
  if (!slave_ptr) {
    return Status::NotFound("ip " + ip + " port " + std::to_string(port));
  }

  {
    std::lock_guard l(slave_ptr->slave_mu);
    if(offset >= GetPreparedId()){
      slave_ptr->slave_state = kSlaveBinlogSync;
    }else {
      slave_ptr->slave_state = KCandidate;
    }
    if(slave_ptr->slave_state == KCandidate){
      LOG(INFO)<<"PacificA first binlog slave_state is Candidate";
    }
    slave_ptr->sent_offset = offset;
    slave_ptr->acked_offset = offset;
    slave_ptr->target_offset =GetPreparedId();
    Status s = slave_ptr->InitBinlogFileReader(Logger(), offset.b_offset);
    if (!s.ok()) {
      return Status::Corruption("Init binlog file reader failed" + s.ToString());  // 如果初始化失败，返回错误状态
    }
    g_pika_rm->DropItemInOneWriteQueue(ip, port, slave_ptr->DBName());
    slave_ptr->b_state = kReadFromFile;
  }

  Status s = coordinator_.SendBinlog(slave_ptr, slave_ptr->DBName());
  if (!s.ok()) {
    return s;
  }

  return Status::OK();
}

/* 
pstd::Status SyncMasterDB::WaitForSlaveAcks(const LogOffset& target_offset, int timeout_ms) {
  // bug: 重发有问题
  // Get slave count
  int slave_count = GetNumberOfSlaveNode();
  LOG(INFO) << "WaitForSlaveAcks: Waiting for ACKs from master and " << slave_count << " slave(s) for target " << target_offset.ToString();

  // If no slaves, return success immediately
  if (slave_count == 0) {
    LOG(INFO) << "WaitForSlaveAcks: No slaves connected, returning success immediately";
    // Update committed_id
    coordinator_.SetCommittedId(target_offset);
    return Status::OK();
  }
  
  g_pika_rm->WakeUpBinlogSync();

  // Use efficient polling mechanism to avoid creating extra threads
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::milliseconds(timeout_ms);
  const int POLL_INTERVAL_MS = 10; // 10ms polling interval
  
  while (true) {
    // Check if timeout
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    if (elapsed >= timeout_duration) {
      LOG(WARNING) << "WaitForSlaveAcks:   after " << timeout_ms << "ms waiting for target " << target_offset.ToString();
      return Status::Timeout("Strong consistency replication timed out");
    }
    
    // Check acknowledgment status of each slave node
    std::unordered_map<std::string, std::shared_ptr<SlaveNode>> slaves = GetAllSlaveNodes();
    int ack_count = 1; // Master node is already acknowledged
    
    for (const auto& slave_pair : slaves) {
      std::shared_ptr<SlaveNode> slave = slave_pair.second;
      if (slave) {
        slave->Lock();
        LogOffset acked_offset = slave->acked_offset;
        slave->Unlock();
        
        if (acked_offset >= target_offset) {
          ack_count++;
        }
      }
    }
    
    // Check if all nodes have acknowledged (1 master + N slaves)
    int expected_acks = 1 + slave_count;
    if (ack_count >= expected_acks) {
      LOG(INFO) << "WaitForSlaveAcks: All " << expected_acks << " nodes have acknowledged target " << target_offset.ToString();
      // Update committed_id
      coordinator_.SetCommittedId(target_offset);
      return Status::OK();
    }
    
    // Sleep briefly then retry
    std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
    
    // Periodically trigger binlog sync to ensure slave nodes receive data
    if (elapsed.count() % 100 == 0) { // Trigger every 100ms
      g_pika_rm->WakeUpBinlogSync();
    }
  }
}
*/

Status SyncMasterDB::ConsensusProposeLog(const std::shared_ptr<Cmd>& cmd_ptr) {
    // If consistency is not required, directly propose the log without waiting for consensus
    if (!coordinator_.GetISConsistency()) {
        return coordinator_.ProposeLog(cmd_ptr);
    }

    LogOffset offset;
    Status s = coordinator_.AppendEntries(cmd_ptr, offset); // Append the log entry to the coordinator

    if (!s.ok()) {
        return s;
    }

    // // Wait for consensus to be achieved within 10 seconds
    // while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() < 10) {
    //     // Check if consensus has been achieved for the given log offset
    //     if (checkFinished(offset)) {
    //         return Status::OK();
    //     }
    //     // TODO: 这里暂时注掉了sleep等待，50ms耗时过长，影响写入链路，后期需要改成条件变量唤醒方式
    //     //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // }

    // return Status::Timeout("No consistency achieved within 10 seconds");
    //LOG(INFO) << "ConsensusProposeLog: Successfully appended cmd " << cmd_ptr->name() 
    //          << " with offset " << offset.ToString() << ", delegating to RocksDBThreadLoop";

    return Status::OK();
}

Status SyncMasterDB::ConsensusProcessLeaderLog(const std::shared_ptr<Cmd>& cmd_ptr, const BinlogItem& attribute) {
  return coordinator_.ProcessLeaderLog(cmd_ptr, attribute);
}

LogOffset SyncMasterDB::ConsensusCommittedIndex() { return coordinator_.committed_index(); }

LogOffset SyncMasterDB::ConsensusLastIndex() { return coordinator_.MemLogger()->last_offset(); }

std::shared_ptr<SlaveNode> SyncMasterDB::GetSlaveNode(const std::string& ip, int port) {
  return coordinator_.SyncPros().GetSlaveNode(ip, port);
}

std::unordered_map<std::string, std::shared_ptr<SlaveNode>> SyncMasterDB::GetAllSlaveNodes() {
  return coordinator_.SyncPros().GetAllSlaveNodes();
}

std::shared_ptr<PikaCommandCollector> SyncMasterDB::GetCommandCollector() { return command_collector_; }

/* SyncSlaveDB */
SyncSlaveDB::SyncSlaveDB(const std::string& db_name)
    : SyncDB(db_name) {
  std::string dbsync_path = g_pika_conf->db_sync_path() + "/" + db_name;
  rsync_cli_.reset(new rsync::RsyncClient(dbsync_path, db_name));
  m_info_.SetLastRecvTime(pstd::NowMicros());
}

void SyncSlaveDB::SetReplState(const ReplState& repl_state) {
  if (repl_state == ReplState::kNoConnect) {
    Deactivate();
    return;
  }
  std::lock_guard l(db_mu_);
  repl_state_ = repl_state;
}

ReplState SyncSlaveDB::State() {
  std::lock_guard l(db_mu_);
  return repl_state_;
}

void SyncSlaveDB::SetLastRecvTime(uint64_t time) {
  std::lock_guard l(db_mu_);
  m_info_.SetLastRecvTime(time);
}

Status SyncSlaveDB::CheckSyncTimeout(uint64_t now) {
  std::lock_guard l(db_mu_);
  // no need to do session keepalive return ok
  if (repl_state_ != ReplState::kWaitDBSync && repl_state_ != ReplState::kConnected) {
    return Status::OK();
  }
  if (m_info_.LastRecvTime() + kRecvKeepAliveTimeout < now) {
    // update slave state to kTryConnect, and try reconnect to master node
    repl_state_ = ReplState::kTryConnect;
  }
  return Status::OK();
}

Status SyncSlaveDB::GetInfo(std::string* info) {
  std::string tmp_str = "  Role: Slave\r\n";
  tmp_str += "  master: " + MasterIp() + ":" + std::to_string(MasterPort()) + "\r\n";
  tmp_str += "  slave status: " + ReplStateMsg[repl_state_] + "\r\n";
  info->append(tmp_str);
  return Status::OK();
}

void SyncSlaveDB::Activate(const RmNode& master, const ReplState& repl_state) {
  std::lock_guard l(db_mu_);
  m_info_ = master;
  repl_state_ = repl_state;
  m_info_.SetLastRecvTime(pstd::NowMicros());
}

void SyncSlaveDB::Deactivate() {
  std::lock_guard l(db_mu_);
  m_info_ = RmNode();
  repl_state_ = ReplState::kNoConnect;
  rsync_cli_->Stop();
}

std::string SyncSlaveDB::ToStringStatus() {
  return "  Master: " + MasterIp() + ":" + std::to_string(MasterPort()) + "\r\n" +
         "  SessionId: " + std::to_string(MasterSessionId()) + "\r\n" + "  SyncStatus " + ReplStateMsg[repl_state_] +
         "\r\n";
}

const std::string& SyncSlaveDB::MasterIp() {
  std::lock_guard l(db_mu_);
  return m_info_.Ip();
}

int SyncSlaveDB::MasterPort() {
  std::lock_guard l(db_mu_);
  return m_info_.Port();
}

void SyncSlaveDB::SetMasterSessionId(int32_t session_id) {
  std::lock_guard l(db_mu_);
  m_info_.SetSessionId(session_id);
}

int32_t SyncSlaveDB::MasterSessionId() {
  std::lock_guard l(db_mu_);
  return m_info_.SessionId();
}

void SyncSlaveDB::SetLocalIp(const std::string& local_ip) {
  std::lock_guard l(db_mu_);
  local_ip_ = local_ip;
}

std::string SyncSlaveDB::LocalIp() {
  std::lock_guard l(db_mu_);
  return local_ip_;
}

void SyncSlaveDB::StopRsync() {
  rsync_cli_->Stop();
}

pstd::Status SyncSlaveDB::ActivateRsync() {
  Status s = Status::OK();
  if (!rsync_cli_->IsIdle()) {
    return s;
  }
  LOG(WARNING) << "Slave DB: " << DBName() << " Activating Rsync ... (retry count:" << rsync_init_retry_count_ << ")";
  if (rsync_cli_->Init()) {
    rsync_init_retry_count_ = 0;
    rsync_cli_->Start();
    return s;
  } else {
    rsync_init_retry_count_ += 1;
    if (rsync_init_retry_count_ >= kMaxRsyncInitReTryTimes) {
      SetReplState(ReplState::kError);
      LOG(ERROR) << "Full Sync Stage - Rsync Init failed: Slave failed to pull meta info(generated by bgsave task in Master) from Master after MaxRsyncInitReTryTimes("
                 << kMaxRsyncInitReTryTimes << " times) is reached. This usually means the Master's bgsave task has costed an unexpected-long time.";
    }
    return Status::Error("rsync client init failed!");
  }
}

/* PikaReplicaManger */

PikaReplicaManager::PikaReplicaManager() {
  std::set<std::string> ips;
  ips.insert("0.0.0.0");
  int port = g_pika_conf->port() + kPortShiftReplServer;
  pika_repl_client_ = std::make_unique<PikaReplClient>(3000, 60);
  pika_repl_server_ = std::make_unique<PikaReplServer>(ips, port, 3000);
  command_queue_ = std::make_unique<CommandQueue>(500);
  // Initialize CommittedID tracking
  last_committed_id_ = LogOffset();
  InitDB();
}

void PikaReplicaManager::Start() {
  int ret = 0;
  ret = pika_repl_client_->Start();
  if (ret != net::kSuccess) {
    LOG(FATAL) << "Start Repl Client Error: " << ret
               << (ret == net::kCreateThreadError ? ": create thread error " : ": other error");
  }

  ret = pika_repl_server_->Start();
  if (ret != net::kSuccess) {
    LOG(FATAL) << "Start Repl Server Error: " << ret
               << (ret == net::kCreateThreadError ? ": create thread error " : ": other error");
  }
      // Start command queue background thread
    StartCommandQueueThread();
    // Start RocksDB background thread
    StartRocksDBThread();
}

void PikaReplicaManager::Stop() {
  // Stop RocksDB background thread first
  StopRocksDBThread();
  // Stop command queue background thread
  StopCommandQueueThread();
  pika_repl_client_->Stop();
  pika_repl_server_->Stop();
}

bool PikaReplicaManager::CheckMasterSyncFinished() {
  for (auto& iter : sync_master_dbs_) {
    std::shared_ptr<SyncMasterDB> db = iter.second;
    LogOffset commit = db->ConsensusCommittedIndex();
    BinlogOffset binlog;
    Status s = db->StableLogger()->Logger()->GetProducerStatus(&binlog.filenum, &binlog.offset);
    if (!s.ok()) {
      return false;
    }
    if (commit.b_offset < binlog) {
      return false;
    }
  }
  return true;
}

void PikaReplicaManager::InitDB() {
  std::vector<DBStruct> db_structs = g_pika_conf->db_structs();
  for (const auto& db : db_structs) {
    const std::string& db_name = db.db_name;
    sync_master_dbs_[DBInfo(db_name)] = std::make_shared<SyncMasterDB>(db_name);
    sync_slave_dbs_[DBInfo(db_name)] = std::make_shared<SyncSlaveDB>(db_name);
  }
}

void PikaReplicaManager::ProduceWriteQueue(const std::string& ip, int port, std::string db_name,
                                           const std::vector<WriteTask>& tasks) {
  std::lock_guard l(write_queue_mu_);
  std::string index = ip + ":" + std::to_string(port);
  LOG(INFO) << "[Queue] Entering ProduceWriteQueue for " << ip << ":" << port << ", db: " << db_name << ", current queue size: " << write_queues_.size() << ", tasks to add: " << tasks.size();
  for (auto& task : tasks) {
    write_queues_[index][db_name].push(task);
  }
  LOG(INFO) << "[Queue] Added " << tasks.size() << " tasks to queue for " << ip << ":" << port << ", db: " << db_name << ", new queue size: " << write_queues_[index][db_name].size();
  LOG(INFO) << "[Queue] Exiting ProduceWriteQueue";
}

int PikaReplicaManager::ConsumeWriteQueue() {
  std::unordered_map<std::string, std::vector<std::vector<WriteTask>>> to_send_map;
  int counter = 0;
  {
    std::lock_guard l(write_queue_mu_);
    for (auto& iter : write_queues_) {
      const std::string& ip_port = iter.first;
      std::map<std::string, std::queue<WriteTask>>& p_map = iter.second;
      for (auto& db_queue : p_map) {
        std::queue<WriteTask>& queue = db_queue.second;
        for (int i = 0; i < kBinlogSendPacketNum; ++i) {
          if (queue.empty()) {
            break;
          }
          size_t batch_index = queue.size() > kBinlogSendBatchNum ? kBinlogSendBatchNum : queue.size();
          std::vector<WriteTask> to_send;
          size_t batch_size = 0;
          for (size_t i = 0; i < batch_index; ++i) {
            WriteTask& task = queue.front();
            batch_size += task.binlog_chip_.binlog_.size();
            // make sure SerializeToString will not over 2G
            if (batch_size > PIKA_MAX_CONN_RBUF_HB) {
              break;
            }
            to_send.push_back(task);
            queue.pop();
            counter++;
          }
          if (!to_send.empty()) {
            to_send_map[ip_port].push_back(std::move(to_send));
          }
        }
      }
    }
  }

  std::vector<std::string> to_delete;
  for (auto& iter : to_send_map) {
    std::string ip;
    int port = 0;
    if (!pstd::ParseIpPortString(iter.first, ip, port)) {
      LOG(WARNING) << "Parse ip_port error " << iter.first;
      continue;
    }
    for (auto& to_send : iter.second) {
      Status s = pika_repl_server_->SendSlaveBinlogChips(ip, port, to_send);
      if (!s.ok()) {
        LOG(WARNING) << "SendSlaveBinlogChips to " << iter.first << " failed, " << s.ToString();
        to_delete.push_back(iter.first);
        continue;
      } else {
        LOG(INFO) << "[Queue] SendSlaveBinlogChips to " << iter.first << " success.";
      }
    }
  }

  if (!to_delete.empty()) {
    std::lock_guard l(write_queue_mu_);
    for (auto& del_queue : to_delete) {
      write_queues_.erase(del_queue);
    }
  }
  // LOG(INFO) << "[Consume] Entering ConsumeWriteQueue, total queues: " << write_queues_.size();
  // LOG(INFO) << "[Consume] Consumed " << counter << " tasks";
  // LOG(INFO) << "[Consume] Exiting ConsumeWriteQueue";
  return counter;
}

void PikaReplicaManager::DropItemInOneWriteQueue(const std::string& ip, int port, const std::string& db_name) {
    std::lock_guard l(write_queue_mu_);
    std::string index = ip + ":" + std::to_string(port);
    if (write_queues_.find(index) != write_queues_.end()) {
      write_queues_[index].erase(db_name);
    }
}

void PikaReplicaManager::DropItemInWriteQueue(const std::string& ip, int port) {
  std::lock_guard l(write_queue_mu_);
  std::string index = ip + ":" + std::to_string(port);
  write_queues_.erase(index);
}

void PikaReplicaManager::ScheduleReplServerBGTask(net::TaskFunc func, void* arg) {
  pika_repl_server_->Schedule(func, arg);
}

void PikaReplicaManager::ScheduleReplClientBGTask(net::TaskFunc func, void* arg) {
  pika_repl_client_->Schedule(func, arg);
}

void PikaReplicaManager::ScheduleReplClientBGTaskByDBName(net::TaskFunc func, void* arg, const std::string &db_name) {
  pika_repl_client_->ScheduleByDBName(func, arg, db_name);
}

void PikaReplicaManager::ScheduleWriteBinlogTask(const std::string& db,
                                                 const std::shared_ptr<InnerMessage::InnerResponse>& res,
                                                 const std::shared_ptr<net::PbConn>& conn, void* res_private_data) {
  pika_repl_client_->ScheduleWriteBinlogTask(db, res, conn, res_private_data);
}

void PikaReplicaManager::ScheduleWriteDBTask(const std::shared_ptr<Cmd>& cmd_ptr, const std::string& db_name) {
  pika_repl_client_->ScheduleWriteDBTask(cmd_ptr, db_name);
}

void PikaReplicaManager::ReplServerRemoveClientConn(int fd) { pika_repl_server_->RemoveClientConn(fd); }

void PikaReplicaManager::ReplServerUpdateClientConnMap(const std::string& ip_port, int fd) {
  pika_repl_server_->UpdateClientConnMap(ip_port, fd);
}

Status PikaReplicaManager::UpdateSyncBinlogStatus(const RmNode& slave, const LogOffset& offset_start,
                                                  const LogOffset& offset_end) {
  std::shared_lock l(dbs_rw_);
  if (sync_master_dbs_.find(slave.NodeDBInfo()) == sync_master_dbs_.end()) {
    return Status::NotFound(slave.ToString() + " not found");
  }
  std::shared_ptr<SyncMasterDB> db = sync_master_dbs_[slave.NodeDBInfo()];
  Status s = db->ConsensusUpdateSlave(slave.Ip(), slave.Port(), offset_start, offset_end);
  if (!s.ok()) {
    return s;
  }
  if(db->GetISConsistency()){
    s = db->UpdateCommittedID();
    if (!s.ok()) {
      return s;
    }
  }
  s = db->SyncBinlogToWq(slave.Ip(), slave.Port());
  if (!s.ok()) {
    return s;
  }
  return Status::OK();
}

bool PikaReplicaManager::CheckSlaveDBState(const std::string& ip, const int port) {
  std::shared_ptr<SyncSlaveDB> db = nullptr;
  for (const auto& iter : g_pika_rm->sync_slave_dbs_) {
    db = iter.second;
    if (db->State() == ReplState::kDBNoConnect && db->MasterIp() == ip &&
        db->MasterPort() + kPortShiftReplServer == port) {
      LOG(INFO) << "DB: " << db->SyncDBInfo().ToString()
                << " has been dbslaveof no one, then will not try reconnect.";
      return false;
    }
  }
  return true;
}

Status PikaReplicaManager::DeactivateSyncSlaveDB(const std::string& ip, int port) {
  std::shared_lock l(dbs_rw_);
  for (auto& iter : sync_slave_dbs_) {
    std::shared_ptr<SyncSlaveDB> db = iter.second;
    if (db->MasterIp() == ip && db->MasterPort() == port) {
      db->Deactivate();
    }
  }
  return Status::OK();
}

Status PikaReplicaManager::LostConnection(const std::string& ip, int port) {
  std::shared_lock l(dbs_rw_);
  for (auto& iter : sync_master_dbs_) {
    std::shared_ptr<SyncMasterDB> db = iter.second;
    Status s = db->RemoveSlaveNode(ip, port);
    if (!s.ok() && !s.IsNotFound()) {
      LOG(WARNING) << "Lost Connection failed " << s.ToString();
    }
  }

  for (auto& iter : sync_slave_dbs_) {
    std::shared_ptr<SyncSlaveDB> db = iter.second;
    if (db->MasterIp() == ip && db->MasterPort() == port) {
      db->Deactivate();
    }
  }
  return Status::OK();
}

Status PikaReplicaManager::WakeUpBinlogSync() {
  std::shared_lock l(dbs_rw_);
  for (auto& iter : sync_master_dbs_) {
    std::shared_ptr<SyncMasterDB> db = iter.second;
    Status s = db->WakeUpSlaveBinlogSync();
    if (!s.ok()) {
      return s;
    }
  }
  return Status::OK();
}

Status PikaReplicaManager::CheckSyncTimeout(uint64_t now) {
  std::shared_lock l(dbs_rw_);

  for (auto& iter : sync_master_dbs_) {
    std::shared_ptr<SyncMasterDB> db = iter.second;
    Status s = db->CheckSyncTimeout(now);
    if (!s.ok()) {
      LOG(WARNING) << "CheckSyncTimeout Failed " << s.ToString();
    }
  }
  for (auto& iter : sync_slave_dbs_) {
    std::shared_ptr<SyncSlaveDB> db = iter.second;
    Status s = db->CheckSyncTimeout(now);
    if (!s.ok()) {
      LOG(WARNING) << "CheckSyncTimeout Failed " << s.ToString();
    }
  }
  return Status::OK();
}

Status PikaReplicaManager::CheckDBRole(const std::string& db, int* role) {
  std::shared_lock l(dbs_rw_);
  *role = 0;
  DBInfo p_info(db);
  if (sync_master_dbs_.find(p_info) == sync_master_dbs_.end()) {
    return Status::NotFound(db  + " not found");
  }
  if (sync_slave_dbs_.find(p_info) == sync_slave_dbs_.end()) {
    return Status::NotFound(db + " not found");
  }
  if (sync_master_dbs_[p_info]->GetNumberOfSlaveNode() != 0 ||
      (sync_master_dbs_[p_info]->GetNumberOfSlaveNode() == 0 &&
       sync_slave_dbs_[p_info]->State() == kNoConnect)) {
    *role |= PIKA_ROLE_MASTER;
  }
  if (sync_slave_dbs_[p_info]->State() != ReplState::kNoConnect) {
    *role |= PIKA_ROLE_SLAVE;
  }
  // if role is not master or slave, the rest situations are all single
  return Status::OK();
}

Status PikaReplicaManager::SelectLocalIp(const std::string& remote_ip, const int remote_port,
                                         std::string* const local_ip) {
  std::unique_ptr<net::NetCli> cli(net::NewRedisCli());
  cli->set_connect_timeout(1500);
  if ((cli->Connect(remote_ip, remote_port, "")).ok()) {
    struct sockaddr_in laddr;
    socklen_t llen = sizeof(laddr);
    getsockname(cli->fd(), reinterpret_cast<struct sockaddr*>(&laddr), &llen);
    std::string tmp_ip(inet_ntoa(laddr.sin_addr));
    *local_ip = tmp_ip;
    cli->Close();
  } else {
    LOG(WARNING) << "Failed to connect remote node(" << remote_ip << ":" << remote_port << ")";
    return Status::Corruption("connect remote node error");
  }
  return Status::OK();
}

Status PikaReplicaManager::ActivateSyncSlaveDB(const RmNode& node, const ReplState& repl_state) {
  std::shared_lock l(dbs_rw_);
  const DBInfo& p_info = node.NodeDBInfo();
  if (sync_slave_dbs_.find(p_info) == sync_slave_dbs_.end()) {
    return Status::NotFound("Sync Slave DB " + node.ToString() + " not found");
  }
  ReplState ssp_state = sync_slave_dbs_[p_info]->State();
  if (ssp_state != ReplState::kNoConnect && ssp_state != ReplState::kDBNoConnect) {
    return Status::Corruption("Sync Slave DB in " + ReplStateMsg[ssp_state]);
  }
  std::string local_ip;
  Status s = SelectLocalIp(node.Ip(), node.Port(), &local_ip);
  if (s.ok()) {
    sync_slave_dbs_[p_info]->SetLocalIp(local_ip);
    sync_slave_dbs_[p_info]->Activate(node, repl_state);
  }
  return s;
}

Status PikaReplicaManager::SendMetaSyncRequest() {
  Status s;
  if (time(nullptr) - g_pika_server->GetMetaSyncTimestamp() >= PIKA_META_SYNC_MAX_WAIT_TIME ||
      g_pika_server->IsFirstMetaSync()) {
    s = pika_repl_client_->SendMetaSync();
    if (s.ok()) {
      g_pika_server->UpdateMetaSyncTimestamp();
      g_pika_server->SetFirstMetaSync(false);
    }
  }
  return s;
}

Status PikaReplicaManager::SendRemoveSlaveNodeRequest(const std::string& db) {
  pstd::Status s;
  std::shared_lock l(dbs_rw_);
  DBInfo p_info(db);
  if (sync_slave_dbs_.find(p_info) == sync_slave_dbs_.end()) {
    return Status::NotFound("Sync Slave DB " + p_info.ToString());
  } else {
    std::shared_ptr<SyncSlaveDB> s_db = sync_slave_dbs_[p_info];
    s = pika_repl_client_->SendRemoveSlaveNode(s_db->MasterIp(), s_db->MasterPort(), db, s_db->LocalIp());
    if (s.ok()) {
      s_db->SetReplState(ReplState::kDBNoConnect);
    }
  }

  if (s.ok()) {
    LOG(INFO) << "SlaveNode (" << db << ", stop sync success";
  } else {
    LOG(WARNING) << "SlaveNode (" << db << ", stop sync faild, " << s.ToString();
  }
  return s;
}

Status PikaReplicaManager::SendTrySyncRequest(const std::string& db_name) {
  BinlogOffset boffset;
  if (!g_pika_server->GetDBBinlogOffset(db_name, &boffset)) {
    LOG(WARNING) << "DB: " << db_name << ", Get DB binlog offset failed";
    return Status::Corruption("DB get binlog offset error");
  }

  std::shared_ptr<SyncSlaveDB> slave_db = GetSyncSlaveDBByName(DBInfo(db_name));
  if (!slave_db) {
    LOG(WARNING) << "Slave DB: " << db_name << ", NotFound";
    return Status::Corruption("Slave DB not found");
  }

  Status status =
      pika_repl_client_->SendTrySync(slave_db->MasterIp(), slave_db->MasterPort(), db_name,
                                     boffset, slave_db->LocalIp());

  if (status.ok()) {
    slave_db->SetReplState(ReplState::kWaitReply);
  } else {
    slave_db->SetReplState(ReplState::kError);
    LOG(WARNING) << "SendDBTrySyncRequest failed " << status.ToString();
  }
  return status;
}

Status PikaReplicaManager::SendDBSyncRequest(const std::string& db_name) {
  BinlogOffset boffset;
  if (!g_pika_server->GetDBBinlogOffset(db_name, &boffset)) {
    LOG(WARNING) << "DB: " << db_name << ", Get DB binlog offset failed";
    return Status::Corruption("DB get binlog offset error");
  }

  std::shared_ptr<DB> db = g_pika_server->GetDB(db_name);
  if (!db) {
    LOG(WARNING) << "DB: " << db_name << " NotFound";
    return Status::Corruption("DB not found");
  }
  db->PrepareRsync();

  std::shared_ptr<SyncSlaveDB> slave_db = GetSyncSlaveDBByName(DBInfo(db_name));
  if (!slave_db) {
    LOG(WARNING) << "Slave DB: " << db_name << ", NotFound";
    return Status::Corruption("Slave DB not found");
  }

  Status status = pika_repl_client_->SendDBSync(slave_db->MasterIp(), slave_db->MasterPort(),
                                                    db_name, boffset, slave_db->LocalIp());

  Status s;
  if (status.ok()) {
    slave_db->SetReplState(ReplState::kWaitReply);
  } else {
    slave_db->SetReplState(ReplState::kError);
    LOG(WARNING) << "SendDBSync failed " << status.ToString();
  }
  if (!s.ok()) {
    LOG(WARNING) << s.ToString();
  }
  return status;
}

Status PikaReplicaManager::SendBinlogSyncAckRequest(const std::string& db, const LogOffset& ack_start,
                                                    const LogOffset& ack_end, bool is_first_send) {
  std::shared_ptr<SyncSlaveDB> slave_db = GetSyncSlaveDBByName(DBInfo(db));
  if (!slave_db) {
    LOG(WARNING) << "Slave DB: " << db << ":, NotFound";
    return Status::Corruption("Slave DB not found");
  }
  return pika_repl_client_->SendBinlogSync(slave_db->MasterIp(), slave_db->MasterPort(), db,
                                           ack_start, ack_end, slave_db->LocalIp(), is_first_send);
}

Status PikaReplicaManager::CloseReplClientConn(const std::string& ip, int32_t port) {
  return pika_repl_client_->Close(ip, port);
}

Status PikaReplicaManager::SendSlaveBinlogChipsRequest(const std::string& ip, int port,
                                                       const std::vector<WriteTask>& tasks) {
  return pika_repl_server_->SendSlaveBinlogChips(ip, port, tasks);
}

std::shared_ptr<SyncMasterDB> PikaReplicaManager::GetSyncMasterDBByName(const DBInfo& p_info) {
  std::shared_lock l(dbs_rw_);
  if (sync_master_dbs_.find(p_info) == sync_master_dbs_.end()) {
    return nullptr;
  }
  return sync_master_dbs_[p_info];
}

std::shared_ptr<SyncSlaveDB> PikaReplicaManager::GetSyncSlaveDBByName(const DBInfo& p_info) {
  std::shared_lock l(dbs_rw_);
  if (sync_slave_dbs_.find(p_info) == sync_slave_dbs_.end()) {
    return nullptr;
  }
  return sync_slave_dbs_[p_info];
}

Status PikaReplicaManager::RunSyncSlaveDBStateMachine() {
  std::shared_lock l(dbs_rw_);
  for (const auto& item : sync_slave_dbs_) {
    DBInfo p_info = item.first;
    std::shared_ptr<SyncSlaveDB> s_db = item.second;
    if (s_db->State() == ReplState::kTryConnect) {
      SendTrySyncRequest(p_info.db_name_);
    } else if (s_db->State() == ReplState::kTryDBSync) {
      SendDBSyncRequest(p_info.db_name_);
    } else if (s_db->State() == ReplState::kWaitReply) {
      continue;
    } else if (s_db->State() == ReplState::kWaitDBSync) {
      Status s = s_db->ActivateRsync();
      if (!s.ok()) {
        LOG(WARNING) << "Slave DB: " << s_db->DBName() << " rsync failed! full synchronization will be retried later";
        continue;
      }

      std::shared_ptr<DB> db =
          g_pika_server->GetDB(p_info.db_name_);
      if (db) {
        if (s_db->IsRsyncExited()) {
          db->TryUpdateMasterOffset();
        }
      } else {
        LOG(WARNING) << "DB not found, DB Name: " << p_info.db_name_;
      }
    } else if (s_db->State() == ReplState::kConnected || s_db->State() == ReplState::kNoConnect ||
               s_db->State() == ReplState::kDBNoConnect) {
      continue;
    }
  }
  return Status::OK();
}

void PikaReplicaManager::FindCommonMaster(std::string* master) {
  std::shared_lock l(dbs_rw_);
  std::string common_master_ip;
  int common_master_port = 0;
  for (auto& iter : sync_slave_dbs_) {
    if (iter.second->State() != kConnected) {
      return;
    }
    std::string tmp_ip = iter.second->MasterIp();
    int tmp_port = iter.second->MasterPort();
    if (common_master_ip.empty() && common_master_port == 0) {
      common_master_ip = tmp_ip;
      common_master_port = tmp_port;
    }
    if (tmp_ip != common_master_ip || tmp_port != common_master_port) {
      return;
    }
  }
  if (!common_master_ip.empty() && common_master_port != 0) {
    *master = common_master_ip + ":" + std::to_string(common_master_port);
  }
}

void PikaReplicaManager::RmStatus(std::string* info) {
  std::shared_lock l(dbs_rw_);
  std::stringstream tmp_stream;
  tmp_stream << "Master DB(" << sync_master_dbs_.size() << "):"
             << "\r\n";
  for (auto& iter : sync_master_dbs_) {
    tmp_stream << " DB " << iter.second->SyncDBInfo().ToString() << "\r\n"
               << iter.second->ToStringStatus() << "\r\n";
  }
  tmp_stream << "Slave DB(" << sync_slave_dbs_.size() << "):"
             << "\r\n";
  for (auto& iter : sync_slave_dbs_) {
    tmp_stream << " DB " << iter.second->SyncDBInfo().ToString() << "\r\n"
               << iter.second->ToStringStatus() << "\r\n";
  }
  info->append(tmp_stream.str());
}
void PikaReplicaManager::BuildBinlogOffset(const LogOffset& offset, InnerMessage::BinlogOffset* boffset) {
  boffset->set_filenum(offset.b_offset.filenum);
  boffset->set_offset(offset.b_offset.offset);
  boffset->set_term(offset.l_offset.term);
  boffset->set_index(offset.l_offset.index);
}

// Command Queue related implementations
void PikaReplicaManager::EnqueueCommandBatch(std::shared_ptr<CommandBatch> batch) {
  if (command_queue_) {
    bool success = command_queue_->EnqueueBatch(batch);
    if (success) {
      // Notify the command queue thread that new data is available
      {
        std::lock_guard<std::mutex> lock(command_queue_mutex_);
        command_queue_cv_.notify_one();
      }
    } else {
      LOG(ERROR) << "Failed to enqueue command batch with " << batch->Size() << " commands";
    }
  } else {
    LOG(ERROR) << "Command queue not initialized";
  }
}

std::shared_ptr<CommandBatch> PikaReplicaManager::DequeueCommandBatch() {
  if (command_queue_) {
    return command_queue_->DequeueBatch();
  } else {
    LOG(ERROR) << "Command queue not initialized";
    return nullptr;
  }
}

size_t PikaReplicaManager::GetCommandQueueSize() const {
  if (command_queue_) {
    return command_queue_->Size();
  }
  return 0;
}

bool PikaReplicaManager::IsCommandQueueEmpty() const {
  if (command_queue_) {
    return command_queue_->Empty();
  }
  return true;
}

void PikaReplicaManager::StartCommandQueueThread() {
  if (command_queue_running_.load()) {
    LOG(WARNING) << "Command queue thread is already running";
    return;
  }
  command_queue_running_.store(true);
  command_queue_thread_ = std::make_unique<std::thread>(&PikaReplicaManager::CommandQueueLoop, this);
  LOG(INFO) << "Command queue background thread started";
}

void PikaReplicaManager::StopCommandQueueThread() {
  if (!command_queue_running_.load()) {
    return;
  }
  command_queue_running_.store(false);
  // Notify the condition variable to wake up the thread
  {
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    command_queue_cv_.notify_one();
  }
  
  if (command_queue_thread_ && command_queue_thread_->joinable()) {
    command_queue_thread_->join();
  }
  LOG(INFO) << "Command queue background thread stopped";
}

void PikaReplicaManager::CommandQueueLoop() {
  LOG(INFO) << "Command queue loop started";
  while (command_queue_running_.load()) {
    try {
      std::unique_lock<std::mutex> lock(command_queue_mutex_);
      // Wait for notification or timeout
      command_queue_cv_.wait(lock, [this] {
        return !command_queue_running_.load() || !command_queue_->Empty();
      });
      
      // Check if we should exit
      if (!command_queue_running_.load()) {
        break;
      }
      
      // Release lock before processing
      lock.unlock();
      
      // Continuously process batches until queue is empty
      bool processed_any_batch = false;
      do {
        // Non-blocking dequeue all available batches
        auto batches = command_queue_->DequeueAllBatches();
        
        if (!batches.empty()) {
          // Process all batches
          ProcessCommandBatches(batches);
          processed_any_batch = true;
          LOG(INFO) << "Processed " << batches.size() << " batches, checking for more...";
        } else {
          processed_any_batch = false;
        }
        // Continue processing if we processed any batches (there might be more)
      } while (processed_any_batch && command_queue_running_.load());
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error in command queue loop: " << e.what();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  LOG(INFO) << "Command queue loop ended";
}

void PikaReplicaManager::ProcessCommandBatches(const std::vector<std::shared_ptr<CommandBatch>>& batches) {
  if (batches.empty()) {
    return;
  }
  size_t total_commands = 0;
  for (const auto& batch : batches) {
    total_commands += batch->Size();
  }
  LOG(INFO) << "Processing " << batches.size() << " batches with " << total_commands << " total commands";
  
  // Merge all commands from all batches for unified batch processing
  std::vector<std::shared_ptr<Cmd>> all_commands;
  std::map<size_t, std::pair<std::shared_ptr<CommandBatch>, size_t>> command_to_batch_map;
  size_t global_cmd_idx = 0;
  for (const auto& batch : batches) {
    if (!batch || batch->Empty()) {
      continue;
    }
    for (size_t i = 0; i < batch->commands.size(); ++i) {
      all_commands.push_back(batch->commands[i]);
      command_to_batch_map[global_cmd_idx] = std::make_pair(batch, i);
      global_cmd_idx++;
    }
  }
  
  if (all_commands.empty()) {
    LOG(WARNING) << "No valid commands found in batches";
    return;
  }
  
  // Use the first batch's database for consensus
  std::string db_name = batches[0]->db_name;
  std::shared_ptr<SyncMasterDB> sync_db = GetSyncMasterDBByName(DBInfo(db_name));
  if (!sync_db) {
    LOG(WARNING) << "SyncMasterDB not found for database: " << db_name;
    // Call callbacks with error for all batches
    for (const auto& batch : batches) {
      for (size_t i = 0; i < batch->callbacks.size(); ++i) {
        if (batch->callbacks[i]) {
          batch->callbacks[i](LogOffset(), pstd::Status::NotFound("Database not found"));
        }
      }
    }
    return;
  }
  
  try {
    // Process each command individually using AppendEntries
    std::vector<LogOffset> all_offsets;
    all_offsets.resize(all_commands.size());
    
    // Process each command using the original single-command logic
    for (size_t cmd_idx = 0; cmd_idx < all_commands.size(); ++cmd_idx) {
      const auto& cmd_ptr = all_commands[cmd_idx];
      LogOffset cmd_offset;
      
      pstd::Status s = sync_db->ConsensusProposeLog(cmd_ptr);
      if (!s.ok()) {
        LOG(ERROR) << "Failed to " << (sync_db->GetISConsistency() ? "append" : "propose") 
                   << " command " << cmd_ptr->name() << ": " << s.ToString();
        // Call callbacks with error for remaining commands
        for (size_t error_idx = cmd_idx; error_idx < all_commands.size(); ++error_idx) {
          auto& batch_info = command_to_batch_map[error_idx];
          auto batch = batch_info.first;
          size_t batch_cmd_idx = batch_info.second;
          if (batch_cmd_idx < batch->callbacks.size() && batch->callbacks[batch_cmd_idx]) {
            batch->callbacks[batch_cmd_idx](LogOffset(), s);
          }
        }
        return;
      }
      // Get the prepared ID as the offset
      cmd_offset = sync_db->GetPreparedId();
      
      all_offsets[cmd_idx] = cmd_offset;
      
      // Update individual batch offsets
      auto& batch_info = command_to_batch_map[cmd_idx];
      auto batch = batch_info.first;
      size_t batch_cmd_idx = batch_info.second;
      
      // Ensure the batch has enough space for offsets
      if (batch->binlog_offsets.size() <= batch_cmd_idx) {
        batch->binlog_offsets.resize(batch->commands.size());
      }
      batch->binlog_offsets[batch_cmd_idx] = cmd_offset;
    }
    
    LOG(INFO) << "Successfully processed " << all_commands.size() << " commands individually";
    
    // Create BatchGroup with the last offset as end_offset
    LogOffset end_offset = all_offsets.back();
    auto batch_group = std::make_shared<BatchGroup>(batches, end_offset);
    
    // Enqueue the BatchGroup
    {
      std::lock_guard<std::mutex> lock(pending_batch_groups_mutex_);
      pending_batch_groups_.push(batch_group);
    }
    LOG(INFO) << "Created BatchGroup with " << batches.size() << " batches and end_offset: " << end_offset.ToString();
  } catch (const std::exception& e) {
    LOG(ERROR) << "Exception in ProcessCommandBatches: " << e.what();
    // Call callbacks with error for all batches
    for (const auto& batch : batches) {
      for (size_t i = 0; i < batch->callbacks.size(); ++i) {
        if (batch->callbacks[i]) {
          batch->callbacks[i](LogOffset(), pstd::Status::Corruption("Exception in processing"));
        }
      }
    }
  }
  
  // Signal auxiliary thread once after processing all batches
  g_pika_server->SignalAuxiliary();
}

// RocksDB Background Thread Implementation
void PikaReplicaManager::StartRocksDBThread() {
  if (rocksdb_thread_running_.load()) {
    LOG(WARNING) << "RocksDB background thread is already running";
    return;
  }
  rocksdb_thread_running_.store(true);
  rocksdb_back_thread_ = std::make_unique<std::thread>(&PikaReplicaManager::RocksDBThreadLoop, this);
  LOG(INFO) << "RocksDB background thread started";
}

void PikaReplicaManager::StopRocksDBThread() {
  if (!rocksdb_thread_running_.load()) {
    return;
  }
  rocksdb_thread_running_.store(false);
  // Notify the condition variable to wake up the thread
  {
    std::lock_guard<std::mutex> lock(rocksdb_thread_mutex_);
    rocksdb_thread_cv_.notify_one();
  }
  if (rocksdb_back_thread_ && rocksdb_back_thread_->joinable()) {
    rocksdb_back_thread_->join();
  }
  LOG(INFO) << "RocksDB background thread stopped";
}

void PikaReplicaManager::RocksDBThreadLoop() {
  LOG(INFO) << "RocksDB_back_thread started";
  while (rocksdb_thread_running_.load()) {
    try {
      std::unique_lock<std::mutex> lock(rocksdb_thread_mutex_);
      // Wait for CommittedID notification or shutdown signal
      rocksdb_thread_cv_.wait(lock, [this] {
        bool has_pending;
        {
          std::lock_guard<std::mutex> pending_lock(pending_batch_groups_mutex_);
          has_pending = !pending_batch_groups_.empty();
        }
        return !rocksdb_thread_running_.load() || has_pending;
      });
      // Check if we should exit
      if (!rocksdb_thread_running_.load()) {
        break;
      }
      lock.unlock();
      // Get current committed ID
      LogOffset committed_id;
      {
        std::lock_guard<std::mutex> id_lock(last_committed_id_mutex_);
        committed_id = last_committed_id_;
      }
      
      // Process committed BatchGroups
      if (committed_id.IsValid()) {
        size_t groups_processed = ProcessCommittedBatchGroups(committed_id);
        if (groups_processed > 0) {
          LOG(INFO) << "Processed " << groups_processed << " committed BatchGroups";
        }
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error in RocksDB thread loop: " << e.what();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  LOG(INFO) << "RocksDB_back_thread ended";
}

size_t PikaReplicaManager::ProcessCommittedBatchGroups(const LogOffset& committed_id) {
  std::queue<std::shared_ptr<BatchGroup>> groups_to_process;
  // Get pending BatchGroups and separate committed from uncommitted
  {
    std::lock_guard<std::mutex> lock(pending_batch_groups_mutex_);
    while (!pending_batch_groups_.empty()) {
      auto batch_group = pending_batch_groups_.front();
      // Check if this BatchGroup is committed by comparing its end_offset with committed_id
      bool is_committed = (batch_group->end_offset <= committed_id);
      if (is_committed) {
        groups_to_process.push(batch_group);
        pending_batch_groups_.pop();
        LOG(INFO) << "BatchGroup with end_offset " << batch_group->end_offset.ToString() 
                  << " is committed (committed_id: " << committed_id.ToString() << ")";
      } else {
        // First uncommitted BatchGroup found, stop processing
        static LogOffset last_uncommitted_offset;
        if (last_uncommitted_offset != batch_group->end_offset) {
          LOG(INFO) << "BatchGroup with end_offset " << batch_group->end_offset.ToString() 
                    << " is not yet committed (committed_id: " << committed_id.ToString() << ")";
          last_uncommitted_offset = batch_group->end_offset;
        }
        break;
      }
    }
  }
  // Store the number of groups to process for return value
  size_t groups_count = groups_to_process.size();
  // Process committed BatchGroups
  while (!groups_to_process.empty()) {
    auto batch_group = groups_to_process.front();
    groups_to_process.pop();
    //LOG(INFO) << "Processing committed BatchGroup with " << batch_group->BatchCount() 
    //          << " batches for RocksDB Put operations and client callbacks";
    
    // Process all batches in this group
    for (const auto& batch : batch_group->batches) {
      // Execute RocksDB Put operations for each command in the batch
      for (size_t i = 0; i < batch->commands.size(); ++i) {
        const auto& cmd_ptr = batch->commands[i];
        LOG(INFO) << "[RocksDB] RocksDBThreadLoop" << "Processing BatchGroup with end_offset: " << batch_group->end_offset.ToString();
        try {
          // Execute the command (this will perform the RocksDB Put operation)
          cmd_ptr->Execute();          
          // auto tmp_conn = cmd_ptr->GetConn();
          // auto pc = dynamic_cast<PikaClientConn*>(tmp_conn.get());
          // std::shared_ptr<std::string> resp_ptr = std::make_shared<std::string>();
          // *resp_ptr = std::move(cmd_ptr->res().message());
          // pc->resp_num--;
          // pc->resp_array.push_back(resp_ptr);
          // pc->TryWriteResp();
          if (cmd_ptr->res().ok()) {
            LOG(INFO) << "Successfully executed RocksDB Put for command: " << cmd_ptr->name();
            // Execute callback with BatchGroup's end_offset (all commands in the group get the same offset)
            if (i < batch->callbacks.size() && batch->callbacks[i]) {
              LOG(INFO) << "[Callback] RocksDBThreadLoop" 
                        << ", Executing callback for end_offset: " << batch_group->end_offset.ToString();
              batch->callbacks[i](batch_group->end_offset, pstd::Status::OK());
            }
          } else {
            LOG(WARNING) << "Command " << cmd_ptr->name() << " execution failed: " << cmd_ptr->res().message();
            // Execute callback with command error, still use BatchGroup's end_offset
            if (i < batch->callbacks.size() && batch->callbacks[i]) {
              batch->callbacks[i](batch_group->end_offset, pstd::Status::Corruption("Command execution failed: " + cmd_ptr->res().message()));
            }
          }
        } catch (const std::exception& e) {
          LOG(ERROR) << "Exception during command execution: " << e.what();
          // Execute callback with exception error, still use BatchGroup's end_offset
          if (i < batch->callbacks.size() && batch->callbacks[i]) {
            batch->callbacks[i](batch_group->end_offset, pstd::Status::Corruption("Exception during execution: " + std::string(e.what())));
          }
        }
      }
    }
  }
  
  return groups_count;
}

void PikaReplicaManager::NotifyCommittedID(const LogOffset& committed_id) {
  {
    std::lock_guard<std::mutex> lock(last_committed_id_mutex_);
    last_committed_id_ = committed_id;
  }
  // Notify RocksDB thread
  {
    std::lock_guard<std::mutex> lock(rocksdb_thread_mutex_);
    rocksdb_thread_cv_.notify_one();
  }
  LOG(INFO) << "Notified RocksDB thread of new CommittedID: " << committed_id.ToString();
}
