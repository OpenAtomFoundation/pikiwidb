// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PIKA_RM_H_
#define PIKA_RM_H_

#include <memory>
#include <queue>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "pstd/include/pstd_status.h"

#include "include/pika_binlog_reader.h"
#include "include/pika_consensus.h"
#include "include/pika_repl_client.h"
#include "include/pika_repl_server.h"
#include "include/pika_slave_node.h"
#include "include/pika_stable_log.h"
#include "include/rsync_client.h"
#include "include/pika_command_collector.h"
#include "include/pika_command_queue.h"

#define kBinlogSendPacketNum 40
#define kBinlogSendBatchNum 100

// unit seconds
// WXR
#define kSendKeepAliveTimeout (100 * 1000000)
#define kRecvKeepAliveTimeout (200 * 1000000)


class SyncDB {
 public:
  SyncDB(const std::string& db_name);
  virtual ~SyncDB() = default;
  DBInfo& SyncDBInfo() { return db_info_; }
  std::string DBName();

 protected:
  DBInfo db_info_;
};

class SyncMasterDB : public SyncDB {
 public:
  SyncMasterDB(const std::string& db_name);
  pstd::Status AddSlaveNode(const std::string& ip, int port, int session_id);
  pstd::Status RemoveSlaveNode(const std::string& ip, int port);
  pstd::Status ActivateSlaveBinlogSync(const std::string& ip, int port, const LogOffset& offset);
  pstd::Status ActivateSlaveDbSync(const std::string& ip, int port);
  pstd::Status SyncBinlogToWq(const std::string& ip, int port);
  pstd::Status GetSlaveSyncBinlogInfo(const std::string& ip, int port, BinlogOffset* sent_offset, BinlogOffset* acked_offset);
  pstd::Status GetSlaveState(const std::string& ip, int port, SlaveState* slave_state);
  pstd::Status SetLastRecvTime(const std::string& ip, int port, uint64_t time);
  pstd::Status GetSafetyPurgeBinlog(std::string* safety_purge);
  pstd::Status WakeUpSlaveBinlogSync();
  pstd::Status CheckSyncTimeout(uint64_t now);
  pstd::Status GetSlaveNodeSession(const std::string& ip, int port, int32_t* session);
  int GetNumberOfSlaveNode();
  bool BinlogCloudPurge(uint32_t index);
  bool CheckSlaveNodeExist(const std::string& ip, int port);

  // debug use
  std::string ToStringStatus();
  int32_t GenSessionId();
  bool CheckSessionId(const std::string& ip, int port, const std::string& db_name, int session_id);

  // consensus use
  pstd::Status ConsensusUpdateSlave(const std::string& ip, int port, const LogOffset& start, const LogOffset& end);
  pstd::Status ConsensusProposeLog(const std::shared_ptr<Cmd>& cmd_ptr);
  pstd::Status ConsensusProcessLeaderLog(const std::shared_ptr<Cmd>& cmd_ptr, const BinlogItem& attribute);
  LogOffset ConsensusCommittedIndex();

  LogOffset ConsensusLastIndex();

  std::shared_ptr<StableLog> StableLogger() { return coordinator_.StableLogger(); }

  std::shared_ptr<Binlog> Logger() {
    if (!coordinator_.StableLogger()) {
      return nullptr;
    }
    return coordinator_.StableLogger()->Logger();
  }

  std::shared_ptr<SlaveNode> GetSlaveNode(const std::string& ip, int port);
  // Make coordinator_ accessible to StableLog class
  ConsensusCoordinator& GetCoordinator() { return coordinator_; }
  std::shared_ptr<PikaCommandCollector> GetCommandCollector();

 private:
  // invoker need to hold slave_mu_
  pstd::Status ReadBinlogFileToWq(const std::shared_ptr<SlaveNode>& slave_ptr);

  //std::shared_ptr<SlaveNode> GetSlaveNode(const std::string& ip, int port);
  std::unordered_map<std::string, std::shared_ptr<SlaveNode>> GetAllSlaveNodes();

  pstd::Mutex session_mu_;
  int32_t session_id_ = 0;
  ConsensusCoordinator coordinator_;
  std::shared_ptr<PikaCommandCollector> command_collector_;

  //pacificA public:
 public:
   void InitContext(){
    coordinator_.InitContext();
  }
  bool checkFinished(const LogOffset& offset);
  void SetConsistency(bool is_consistenct);
  bool GetISConsistency();
  pstd::Status ProcessCoordination(); 
  void SetPreparedId(const LogOffset& offset);
  void SetCommittedId(const LogOffset& offset);
  LogOffset GetPreparedId();
  LogOffset GetCommittedId();
  pstd::Status AppendSlaveEntries(const std::shared_ptr<Cmd>& cmd_ptr, const BinlogItem& attribute);
  pstd::Status AppendCandidateBinlog(const std::string& ip, int port, const LogOffset& offset);
  pstd::Status UpdateCommittedID();
  pstd::Status CommitAppLog(const LogOffset& master_committed_id);
  pstd::Status Truncate(const LogOffset& offset);
  // pstd::Status WaitForSlaveAcks(const LogOffset& target_offset, int timeout_ms);
};

class SyncSlaveDB : public SyncDB {
 public:
  SyncSlaveDB(const std::string& db_name);
  void Activate(const RmNode& master, const ReplState& repl_state);
  void Deactivate();
  void SetLastRecvTime(uint64_t time);
  void SetReplState(const ReplState& repl_state);
  ReplState State();
  pstd::Status CheckSyncTimeout(uint64_t now);

  // For display
  pstd::Status GetInfo(std::string* info);
  // For debug
  std::string ToStringStatus();
  std::string LocalIp();
  int32_t MasterSessionId();
  const std::string& MasterIp();
  int MasterPort();
  void SetMasterSessionId(int32_t session_id);
  void SetLocalIp(const std::string& local_ip);
  void StopRsync();
  pstd::Status ActivateRsync();
  bool IsRsyncExited() { return rsync_cli_->IsExitedFromRunning(); }

 private:
  std::unique_ptr<rsync::RsyncClient> rsync_cli_;
  int32_t rsync_init_retry_count_{0};
  pstd::Mutex db_mu_;
  RmNode m_info_;
  ReplState repl_state_{kNoConnect};
  std::string local_ip_;
};

class PikaReplicaManager {
 public:
  PikaReplicaManager();
  ~PikaReplicaManager() = default;
  friend Cmd;
  void Start();
  void Stop();
  bool CheckMasterSyncFinished();
  pstd::Status ActivateSyncSlaveDB(const RmNode& node, const ReplState& repl_state);

  // For Pika Repl Client Thread
  pstd::Status SendMetaSyncRequest();
  pstd::Status SendRemoveSlaveNodeRequest(const std::string& table);
  pstd::Status SendTrySyncRequest(const std::string& db_name);
  pstd::Status SendDBSyncRequest(const std::string& db_name);
  pstd::Status SendBinlogSyncAckRequest(const std::string& table, const LogOffset& ack_start,
                                        const LogOffset& ack_end, bool is_first_send = false);
  pstd::Status CloseReplClientConn(const std::string& ip, int32_t port);

  // For Pika Repl Server Thread
  pstd::Status SendSlaveBinlogChipsRequest(const std::string& ip, int port, const std::vector<WriteTask>& tasks);

  // For SyncMasterDB
  std::shared_ptr<SyncMasterDB> GetSyncMasterDBByName(const DBInfo& p_info);

  // For SyncSlaveDB
  std::shared_ptr<SyncSlaveDB> GetSyncSlaveDBByName(const DBInfo& p_info);

  pstd::Status RunSyncSlaveDBStateMachine();

  pstd::Status CheckSyncTimeout(uint64_t now);

  // To check db info
  // For pkcluster info command
  static bool CheckSlaveDBState(const std::string& ip, int port);
  void FindCommonMaster(std::string* master);
  void RmStatus(std::string* debug_info);
  pstd::Status CheckDBRole(const std::string& table, int* role);
  pstd::Status LostConnection(const std::string& ip, int port);
  pstd::Status DeactivateSyncSlaveDB(const std::string& ip, int port);

  // Update binlog win and try to send next binlog
  pstd::Status UpdateSyncBinlogStatus(const RmNode& slave, const LogOffset& offset_start, const LogOffset& offset_end);
  pstd::Status WakeUpBinlogSync();

  // write_queue related
  void ProduceWriteQueue(const std::string& ip, int port, std::string db_name, const std::vector<WriteTask>& tasks);
  void DropItemInOneWriteQueue(const std::string& ip, int port, const std::string& db_name);
  void DropItemInWriteQueue(const std::string& ip, int port);
  int ConsumeWriteQueue();

  // Schedule Task
  void ScheduleReplServerBGTask(net::TaskFunc func, void* arg);
  void ScheduleReplClientBGTask(net::TaskFunc func, void* arg);
  void ScheduleWriteBinlogTask(const std::string& db_name,
                               const std::shared_ptr<InnerMessage::InnerResponse>& res,
                               const std::shared_ptr<net::PbConn>& conn, void* res_private_data);
  void ScheduleWriteDBTask(const std::shared_ptr<Cmd>& cmd_ptr, const std::string& db_name);
  void ScheduleReplClientBGTaskByDBName(net::TaskFunc , void* arg, const std::string &db_name);
  void ReplServerRemoveClientConn(int fd);
  void ReplServerUpdateClientConnMap(const std::string& ip_port, int fd);

  std::shared_mutex& GetDBLock() { return dbs_rw_; }

  void BuildBinlogOffset(const LogOffset& offset, InnerMessage::BinlogOffset* boffset);

  void DBLock() {
    dbs_rw_.lock();
  }
  void DBUnlock() {
    dbs_rw_.unlock();
  }

  std::unordered_map<DBInfo, std::shared_ptr<SyncMasterDB>, hash_db_info>& GetSyncMasterDBs() {
    return sync_master_dbs_;
  }
  std::unordered_map<DBInfo, std::shared_ptr<SyncSlaveDB>, hash_db_info>& GetSyncSlaveDBs() {
    return sync_slave_dbs_;
  }

  int32_t GetUnfinishedAsyncWriteDBTaskCount(const std::string& db_name) {
    return pika_repl_client_->GetUnfinishedAsyncWriteDBTaskCount(db_name);
  }

  // Command Queue related methods
  void EnqueueCommandBatch(std::shared_ptr<CommandBatch> batch);
  std::shared_ptr<CommandBatch> DequeueCommandBatch();
  size_t GetCommandQueueSize() const;
  bool IsCommandQueueEmpty() const;
  // CommittedID notification for RocksDB thread
  void NotifyCommittedID(const LogOffset& committed_id);

 private:
  void InitDB();
  pstd::Status SelectLocalIp(const std::string& remote_ip, int remote_port, std::string* local_ip);

  std::shared_mutex dbs_rw_;
  std::unordered_map<DBInfo, std::shared_ptr<SyncMasterDB>, hash_db_info> sync_master_dbs_;
  std::unordered_map<DBInfo, std::shared_ptr<SyncSlaveDB>, hash_db_info> sync_slave_dbs_;

  pstd::Mutex write_queue_mu_;

  // db_name -> a queue of write task
  using DBWriteTaskQueue = std::map<std::string, std::queue<WriteTask>>;
  // ip:port -> a map of DBWriteTaskQueue
  using SlaveWriteTaskQueue = std::map<std::string, DBWriteTaskQueue>;

  // every host owns a queue, the key is "ip + port"
  SlaveWriteTaskQueue write_queues_;

  // client for replica
  std::unique_ptr<PikaReplClient> pika_repl_client_;
  std::unique_ptr<PikaReplServer> pika_repl_server_;
 
  // Condition variable for signaling when the write queue has new items
  pstd::CondVar write_queue_cv_;

  std::shared_mutex is_consistency_rwlock_;
  bool is_consistency_ = true;
  std::shared_mutex committed_id_rwlock_;

  // Command queue for collected batches
  std::unique_ptr<CommandQueue> command_queue_;
  
  // Background thread for processing command queue
  std::unique_ptr<std::thread> command_queue_thread_;
  std::atomic<bool> command_queue_running_{false};
  std::mutex command_queue_mutex_;
  std::condition_variable command_queue_cv_;
  
  // RocksDB background thread for Put operations and client responses
  std::unique_ptr<std::thread> rocksdb_back_thread_;
  std::atomic<bool> rocksdb_thread_running_{false};
  std::mutex rocksdb_thread_mutex_;
  std::condition_variable rocksdb_thread_cv_;
  
  // Pending batch groups waiting for CommittedID
  std::queue<std::shared_ptr<BatchGroup>> pending_batch_groups_;
  std::mutex pending_batch_groups_mutex_;
  
  // Last committed ID for RocksDB thread processing
  LogOffset last_committed_id_;
  std::shared_mutex last_committed_id_mutex_;
  
  // Background thread processing methods
  void StartCommandQueueThread();
  void StopCommandQueueThread();
  void CommandQueueLoop();
  void ProcessCommandBatches(const std::vector<std::shared_ptr<CommandBatch>>& batches);
  
  // RocksDB background thread methods
  void StartRocksDBThread();
  void StopRocksDBThread();
  void RocksDBThreadLoop();
  size_t ProcessCommittedBatchGroups(const LogOffset& committed_id);
};

#endif  //  PIKA_RM_H
