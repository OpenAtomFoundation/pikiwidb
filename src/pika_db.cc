// Copyright (c) 2018-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <sys/stat.h>
#include <fstream>
#include <utility>

#include "include/pika_db.h"

#include "include/pika_cmd_table_manager.h"
#include "include/pika_rm.h"
#include "include/pika_server.h"
#include "mutex_impl.h"

using pstd::Status;
extern PikaServer* g_pika_server;
extern std::unique_ptr<PikaReplicaManager> g_pika_rm;
extern std::unique_ptr<PikaCmdTableManager> g_pika_cmd_table_manager;

std::string DBPath(const std::string& path, const std::string& db_name) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%s/", db_name.data());
  return path + buf;
}

std::string DbSyncPath(const std::string& sync_path, const std::string& db_name) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s/", db_name.data());
  return sync_path + buf;
}

DB::DB(std::string db_name, const std::string& db_path,
             const std::string& log_path)
    : db_name_(db_name), bgsave_engine_(nullptr) {
  db_path_ = DBPath(db_path, db_name_);
  bgsave_sub_path_ = db_name;
  dbsync_path_ = DbSyncPath(g_pika_conf->db_sync_path(), db_name);
  log_path_ = DBPath(log_path, "log_" + db_name_);
  storage_ = std::make_shared<storage::Storage>(g_pika_conf->db_instance_num(),
      g_pika_conf->default_slot_num(), g_pika_conf->classic_mode());
  rocksdb::Status s = storage_->Open(g_pika_server->storage_options(), db_path_);
  pstd::CreatePath(db_path_);
  pstd::CreatePath(log_path_);
  lock_mgr_ = std::make_shared<pstd::lock::LockMgr>(1000, 0, std::make_shared<pstd::lock::MutexFactoryImpl>());
  binlog_io_error_.store(false);
  opened_ = s.ok();
  assert(storage_);
  assert(s.ok());
  LOG(INFO) << db_name_ << " DB Success";
}

DB::~DB() {
  StopKeyScan();
}

bool DB::WashData() {
  rocksdb::ReadOptions read_options;
  rocksdb::Status s;
  auto suffix_len = storage::ParsedBaseDataValue::GetkBaseDataValueSuffixLength();
  for (int i = 0; i < g_pika_conf->db_instance_num(); i++) {
    rocksdb::WriteBatch batch;
    auto handle = storage_->GetHashCFHandles(i)[1];
    auto db = storage_->GetDBByIndex(i);
    auto it(db->NewIterator(read_options, handle));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      std::string key = it->key().ToString();
      std::string value = it->value().ToString();
      if (value.size() < suffix_len) {
        // need to wash
        storage::BaseDataValue internal_value(value);
        batch.Put(handle, key, internal_value.Encode());
      }
    }
    delete it;
    s = db->Write(storage_->GetDefaultWriteOptions(i), &batch);
    if (!s.ok()) {
      return false;
    }
  }
  return true;
}

std::string DB::GetDBName() { return db_name_; }

void DB::BgSaveDB() {
  std::shared_lock l(dbs_rw_);
  std::lock_guard ml(bgsave_protector_);
  if (bgsave_info_.bgsaving) {
    return;
  }
  bgsave_info_.bgsaving = true;
  auto bg_task_arg = new BgTaskArg();
  bg_task_arg->db = shared_from_this();
  g_pika_server->BGSaveTaskSchedule(&DoBgSave, static_cast<void*>(bg_task_arg));
}

void DB::SetBinlogIoError() { return binlog_io_error_.store(true); }
void DB::SetBinlogIoErrorrelieve() { return binlog_io_error_.store(false); }
bool DB::IsBinlogIoError() { return binlog_io_error_.load(); }
std::shared_ptr<pstd::lock::LockMgr> DB::LockMgr() { return lock_mgr_; }
std::shared_ptr<PikaCache> DB::cache() const { return cache_; }
std::shared_ptr<storage::Storage> DB::storage() const { return storage_; }

void DB::KeyScan() {
  std::lock_guard ml(key_scan_protector_);
  if (key_scan_info_.key_scaning_) {
    return;
  }

  key_scan_info_.duration = -2;  // duration -2 mean the task in waiting status,
                                 // has not been scheduled for exec
  auto bg_task_arg = new BgTaskArg();
  bg_task_arg->db = shared_from_this();
  g_pika_server->KeyScanTaskSchedule(&DoKeyScan, reinterpret_cast<void*>(bg_task_arg));
}

bool DB::IsKeyScaning() {
  std::lock_guard ml(key_scan_protector_);
  return key_scan_info_.key_scaning_;
}

void DB::RunKeyScan() {
  Status s;
  std::vector<storage::KeyInfo> new_key_infos;

  InitKeyScan();
  std::shared_lock l(dbs_rw_);
  s = GetKeyNum(&new_key_infos);
  key_scan_info_.duration = static_cast<int32_t>(time(nullptr) - key_scan_info_.start_time);

  std::lock_guard lm(key_scan_protector_);
  if (s.ok()) {
    key_scan_info_.key_infos = new_key_infos;
  }
  key_scan_info_.key_scaning_ = false;
}

Status DB::GetKeyNum(std::vector<storage::KeyInfo>* key_info) {
  std::lock_guard l(key_info_protector_);
  if (key_scan_info_.key_scaning_) {
    *key_info = key_scan_info_.key_infos;
    return Status::OK();
  }
  InitKeyScan();
  key_scan_info_.key_scaning_ = true;
  key_scan_info_.duration = -2;  // duration -2 mean the task in waiting status,
                                 // has not been scheduled for exec
  rocksdb::Status s = storage_->GetKeyNum(key_info);
  key_scan_info_.key_scaning_ = false;
  if (!s.ok()) {
    return Status::Corruption(s.ToString());
  }
  key_scan_info_.key_infos = *key_info;
  key_scan_info_.duration = static_cast<int32_t>(time(nullptr) - key_scan_info_.start_time);
  return Status::OK();
}

void DB::StopKeyScan() {
  std::shared_lock rwl(dbs_rw_);
  std::lock_guard ml(key_scan_protector_);

  if (!key_scan_info_.key_scaning_) {
    return;
  }
  storage_->StopScanKeyNum();
  key_scan_info_.key_scaning_ = false;
}

void DB::ScanDatabase(const storage::DataType& type) {
  std::shared_lock l(dbs_rw_);
  storage_->ScanDatabase(type);
}

KeyScanInfo DB::GetKeyScanInfo() {
  std::lock_guard lm(key_scan_protector_);
  return key_scan_info_;
}

void DB::Compact(const storage::DataType& type) {
  std::lock_guard rwl(dbs_rw_);
  if (!opened_) {
    return;
  }
  storage_->Compact(type);
}

void DB::CompactRange(const storage::DataType& type, const std::string& start, const std::string& end) {
  std::lock_guard rwl(dbs_rw_);
  if (!opened_) {
    return;
  }
  storage_->CompactRange(type, start, end);
}

void DB::LongestNotCompactionSstCompact(const storage::DataType& type) {
  std::lock_guard rwl(dbs_rw_);
  if (!opened_) {
    return;
  }
  storage_->LongestNotCompactionSstCompact(type);
}

void DB::DoKeyScan(void* arg) {
  std::unique_ptr <BgTaskArg> bg_task_arg(static_cast<BgTaskArg*>(arg));
  bg_task_arg->db->RunKeyScan();
}

void DB::InitKeyScan() {
  key_scan_info_.start_time = time(nullptr);
  char s_time[32];
  size_t len = strftime(s_time, sizeof(s_time), "%Y-%m-%d %H:%M:%S", localtime(&key_scan_info_.start_time));
  key_scan_info_.s_start_time.assign(s_time, len);
  key_scan_info_.duration = -1;  // duration -1 mean the task in processing
}

void DB::SetCompactRangeOptions(const bool is_canceled) {
  if (!opened_) {
    return;
  }
  storage_->SetCompactRangeOptions(is_canceled);
}

DisplayCacheInfo DB::GetCacheInfo() {
  std::lock_guard l(cache_info_rwlock_);
  return cache_info_;
}

bool DB::FlushDBWithoutLock() {
  std::lock_guard l(bgsave_protector_);
  if (bgsave_info_.bgsaving) {
    return false;
  }

  LOG(INFO) << db_name_ << " Delete old db...";
  storage_.reset();

  std::string dbpath = db_path_;
  if (dbpath[dbpath.length() - 1] == '/') {
    dbpath.erase(dbpath.length() - 1);
  }
  std::string delete_suffix("_deleting_");
  delete_suffix.append(std::to_string(NowMicros()));
  delete_suffix.append("/");
  dbpath.append(delete_suffix);
  auto rename_success = pstd::RenameFile(db_path_, dbpath);
  storage_ = std::make_shared<storage::Storage>(g_pika_conf->db_instance_num(),
      g_pika_conf->default_slot_num(), g_pika_conf->classic_mode());
  rocksdb::Status s = storage_->Open(g_pika_server->storage_options(), db_path_);
  assert(storage_);
  assert(s.ok());
  if (rename_success == -1) {
    //the storage_->Open actually opened old RocksDB instance, so flushdb failed
    LOG(WARNING)  << db_name_ << " FlushDB failed due to rename old db_path_ failed";
    return false;
  }
  LOG(INFO) << db_name_ << " Open new db success";

  g_pika_server->PurgeDir(dbpath);
  return true;
}

void DB::DoBgSave(void* arg) {
  std::unique_ptr<BgTaskArg> bg_task_arg(static_cast<BgTaskArg*>(arg));

  // Do BgSave
  bool success = bg_task_arg->db->RunBgsaveEngine();

  // Some output
  BgSaveInfo info = bg_task_arg->db->bgsave_info();
  std::stringstream info_content;
  std::ofstream out;
  out.open(info.path + "/" + kBgsaveInfoFile, std::ios::in | std::ios::trunc);
  if (out.is_open()) {
    info_content << (time(nullptr) - info.start_time) << "s\n"
                 << g_pika_server->host() << "\n"
                 << g_pika_server->port() << "\n"
                 << info.offset.b_offset.filenum << "\n"
                 << info.offset.b_offset.offset << "\n";
    bg_task_arg->db->snapshot_uuid_ = md5(info_content.str());
    out << info_content.rdbuf();
    out.close();
  }
  if (!success) {
    std::string fail_path = info.path + "_FAILED";
    pstd::RenameFile(info.path, fail_path);
  }
  bg_task_arg->db->FinishBgsave();
}

bool DB::RunBgsaveEngine() {
  // Prepare for Bgsaving
  if (!InitBgsaveEnv() || !InitBgsaveEngine()) {
    ClearBgsave();
    return false;
  }
  LOG(INFO) << db_name_ << " after prepare bgsave";

  BgSaveInfo info = bgsave_info();
  LOG(INFO) << db_name_ << " bgsave_info: path=" << info.path << ",  filenum=" << info.offset.b_offset.filenum
            << ", offset=" << info.offset.b_offset.offset;

  // Use SetBackupContentAndCreate to minimize time window between GetLiveFiles and CreateCheckpoint
  // This reduces the chance of compaction occurring and creating orphan files
  rocksdb::Status s = bgsave_engine_->SetBackupContentAndCreate(info.path);

  if (!s.ok()) {
    LOG(WARNING) << db_name_ << " create new backup failed :" << s.ToString();
    return false;
  }
  LOG(INFO) << db_name_ << " create new backup finished.";

  return true;
}

BgSaveInfo DB::bgsave_info() {
  std::lock_guard l(bgsave_protector_);
  return bgsave_info_;
}

void DB::FinishBgsave() {
  std::lock_guard l(bgsave_protector_);
  bgsave_info_.bgsaving = false;
  g_pika_server->UpdateLastSave(time(nullptr));
}

// Prepare engine, need bgsave_protector protect
// Scheme A: Each slave has exclusive dump, so we need unique dump directories
bool DB::InitBgsaveEnv() {
  std::lock_guard l(bgsave_protector_);
  // Prepare for bgsave dir
  bgsave_info_.start_time = time(nullptr);
  char s_time[32];
  int len = static_cast<int32_t>(strftime(s_time, sizeof(s_time), "%Y%m%d%H%M%S", localtime(&bgsave_info_.start_time)));
  bgsave_info_.s_start_time.assign(s_time, len);

  // Scheme A: Use unique directory name with sequence number
  // Format: dump-YYYYMMDD-NN/db_name where NN is sequence number
  std::string base_path = g_pika_conf->bgsave_path();
  std::string date_str(s_time, 8);
  std::string prefix = g_pika_conf->bgsave_prefix() + date_str;

  // Find first available sequence number
  int seq = 0;
  std::string time_sub_path;
  std::string full_path;
  do {
    time_sub_path = prefix + "-" + std::to_string(seq);
    full_path = base_path + time_sub_path + "/" + bgsave_sub_path_;
    seq++;
  } while (pstd::FileExists(full_path) && seq < 1000);  // Max 1000 dumps per day

  if (seq >= 1000) {
    LOG(ERROR) << db_name_ << " too many dump directories for today";
    return false;
  }

  bgsave_info_.path = full_path;
  LOG(INFO) << db_name_ << " preparing bgsave dir: " << bgsave_info_.path;

  // Note: In Scheme A, we don't delete existing directories
  // because other slaves may be using them
  // Just create the new path
  if (!PikaServer::EnsureDirExists(bgsave_info_.path, 0755)) {
    LOG(WARNING) << db_name_ << " create bgsave dir failed: " << bgsave_info_.path
                 << ", errno=" << errno << ", error=" << strerror(errno);
    // Clear the path on failure to avoid using invalid path in GetDumpMeta
    bgsave_info_.path.clear();
    return false;
  }

  // Prepare for failed dir
  std::string failed_dir = bgsave_info_.path + "_FAILED";
  if (pstd::FileExists(failed_dir)) {
    pstd::DeleteDirIfExist(failed_dir);
  }
  return true;
}

// Prepare bgsave env, need bgsave_protector protect
// Note: SetBackupContent is now done in RunBgsaveEngine using SetBackupContentAndCreate
// to minimize time window between GetLiveFiles and CreateCheckpoint
bool DB::InitBgsaveEngine() {
  bgsave_engine_.reset();
  rocksdb::Status s = storage::BackupEngine::Open(storage().get(), bgsave_engine_, g_pika_conf->db_instance_num());
  if (!s.ok()) {
    LOG(WARNING) << db_name_ << " open backup engine failed " << s.ToString();
    return false;
  }

  std::shared_ptr<SyncMasterDB> db =
      g_pika_rm->GetSyncMasterDBByName(DBInfo(db_name_));
  if (!db) {
    LOG(WARNING) << db_name_ << " not found";
    return false;
  }

  {
    std::lock_guard lock(dbs_rw_);
    LogOffset bgsave_offset;
    // term, index are 0
    db->Logger()->GetProducerStatus(&(bgsave_offset.b_offset.filenum), &(bgsave_offset.b_offset.offset));
    {
      std::lock_guard l(bgsave_protector_);
      bgsave_info_.offset = bgsave_offset;
    }
    // SetBackupContent is now done in RunBgsaveEngine to minimize time window
  }
  return true;
}

void DB::Init() {
  cache_ = std::make_shared<PikaCache>(g_pika_conf->zset_cache_start_direction(), g_pika_conf->zset_cache_field_num_per_key());
  // Create cache
  cache::CacheConfig cache_cfg;
  g_pika_server->CacheConfigInit(cache_cfg);
  cache_->Init(g_pika_conf->GetCacheNum(), &cache_cfg);
}

void DB::GetBgSaveMetaData(std::vector<std::string>* fileNames, std::string* snapshot_uuid) {
  const std::string dbPath = bgsave_info().path;
  size_t total_sst_files = 0;
  size_t orphan_sst_files = 0;

  LOG(INFO) << "[GetBgSaveMetaData] Starting scan, dbPath=" << dbPath;

  // dbPath is already the specific DB path (e.g., .../dump/dump-9454-20260302/db0)
  // We need to scan its subdirectories (0, 1, 2 for rocksdb instances)
  std::vector<std::string> subDirs;
  int ret = pstd::GetChildren(dbPath, subDirs);
  LOG(INFO) << "[GetBgSaveMetaData] GetChildren for dbPath returned " << ret
            << ", subDirs count=" << subDirs.size();
  if (ret) {
    LOG(WARNING) << "[GetBgSaveMetaData] Failed to read dbPath: " << dbPath;
    return;
  }

  for (const std::string& subDir : subDirs) {
    std::string instPath = dbPath + "/" + subDir;
    // Skip if not exists or is a file (not directory)
    // Note: IsDir returns 0 for directory, 1 for file, -1 for error
    if (!pstd::FileExists(instPath) || pstd::IsDir(instPath) != 0) {
      continue;
    }

    std::vector<std::string> tmpFileNames;
    ret = pstd::GetChildren(instPath, tmpFileNames);
    if (ret) {
      LOG(WARNING) << "[GetBgSaveMetaData] Failed to read instPath: " << instPath;
      continue;
    }

    for (const std::string& fileName : tmpFileNames) {
      std::string fullPath = instPath + "/" + fileName;
      struct stat st;
      // Check if file exists and get its stat
      if (stat(fullPath.c_str(), &st) != 0) {
        // File doesn't exist, skip it
        LOG(WARNING) << "[GetBgSaveMetaData] File does not exist: " << fullPath;
        continue;
      }

      // Check if it's an SST file and if it's an orphan (Links=1)
      if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".sst") {
        total_sst_files++;
        if (st.st_nlink == 1) {
          // This is an orphan file, but we need to include it in the meta
          // to ensure data consistency. The file will be cleaned up after
          // a delay to allow for retries.
          orphan_sst_files++;
          LOG(INFO) << "[GetBgSaveMetaData] Including orphan SST file: " << fullPath
                    << ", size=" << st.st_size;
          // NOTE: We no longer skip orphan files here. They will be included
          // in the file list and cleaned up with a delay after transfer.
        }
      }
      // Construct relative path like "0/xxx.sst" or "1/xxx.sst"
      fileNames->push_back(subDir + "/" + fileName);
    }
  }

  if (orphan_sst_files > 0) {
    LOG(INFO) << "[GetBgSaveMetaData] Summary for " << dbPath
              << ": total_sst=" << total_sst_files
              << ", orphan_included=" << orphan_sst_files
              << ", returned=" << fileNames->size();
  }

  fileNames->push_back(kBgsaveInfoFile);
  pstd::Status s = GetBgSaveUUID(snapshot_uuid);
  if (!s.ok()) {
    LOG(WARNING) << "read dump meta info failed! error:" << s.ToString();
    return;
  }
}

Status DB::GetBgSaveUUID(std::string* snapshot_uuid) {
  if (snapshot_uuid_.empty()) {
    std::string info_data;
    const std::string infoPath = bgsave_info().path + "/info";
    //TODO: using file read function to replace rocksdb::ReadFileToString
    rocksdb::Status s = rocksdb::ReadFileToString(rocksdb::Env::Default(), infoPath, &info_data);
    if (!s.ok()) {
      LOG(WARNING) << "read dump meta info failed! error:" << s.ToString();
      return Status::IOError("read dump meta info failed", infoPath);
    }
    pstd::MD5 md5 = pstd::MD5(info_data);
    snapshot_uuid_ = md5.hexdigest();
  }
  *snapshot_uuid = snapshot_uuid_;
  return Status::OK();
}

// Try to update master offset
// This may happend when dbsync from master finished
// Here we do:
// 1, Check dbsync finished, got the new binlog offset
// 2, Replace the old db
// 3, Update master offset, and the PikaAuxiliaryThread cron will connect and do slaveof task with master
bool DB::TryUpdateMasterOffset() {
  std::shared_ptr<SyncSlaveDB> slave_db =
      g_pika_rm->GetSyncSlaveDBByName(DBInfo(db_name_));
  if (!slave_db) {
    LOG(ERROR) << "Slave DB: " << db_name_ << " not exist";
    slave_db->SetReplState(ReplState::kError);
    return false;
  }

  std::string info_path = dbsync_path_ + kBgsaveInfoFile;
  if (!pstd::FileExists(info_path)) {
    LOG(WARNING) << "info path: " << info_path << " not exist, Slave DB:" << GetDBName() << " will restart the sync process...";
    // May failed in RsyncClient, thus the complete snapshot dir got deleted
    slave_db->SetReplState(ReplState::kTryConnect);
    return false;
  }

  // Got new binlog offset
  std::ifstream is(info_path);
  if (!is) {
    LOG(WARNING) << "DB: " << db_name_ << ", Failed to open info file after db sync";
    slave_db->SetReplState(ReplState::kError);
    return false;
  }
  std::string line;
  std::string master_ip;
  int lineno = 0;
  int64_t filenum = 0;
  int64_t offset = 0;
  int64_t term = 0;
  int64_t index = 0;
  int64_t tmp = 0;
  int64_t master_port = 0;
  while (std::getline(is, line)) {
    lineno++;
    if (lineno == 2) {
      master_ip = line;
    } else if (lineno > 2 && lineno < 8) {
      if ((pstd::string2int(line.data(), line.size(), &tmp) == 0) || tmp < 0) {
        LOG(WARNING) << "DB: " << db_name_
                     << ", Format of info file after db sync error, line : " << line;
        is.close();
        slave_db->SetReplState(ReplState::kError);
        return false;
      }
      if (lineno == 3) {
        master_port = tmp;
      } else if (lineno == 4) {
        filenum = tmp;
      } else if (lineno == 5) {
        offset = tmp;
      } else if (lineno == 6) {
        term = tmp;
      } else if (lineno == 7) {
        index = tmp;
      }
    } else if (lineno > 8) {
      LOG(WARNING) << "DB: " << db_name_ << ", Format of info file after db sync error, line : " << line;
      is.close();
      slave_db->SetReplState(ReplState::kError);
      return false;
    }
  }
  is.close();

  LOG(INFO) << "DB: " << db_name_ << " Information from dbsync info"
            << ",  master_ip: " << master_ip << ", master_port: " << master_port << ", filenum: " << filenum
            << ", offset: " << offset << ", term: " << term << ", index: " << index;

  pstd::DeleteFile(info_path);
  if (!ChangeDb(dbsync_path_)) {
    LOG(WARNING) << "DB: " << db_name_ << ", Failed to change db";
    slave_db->SetReplState(ReplState::kError);
    return false;
  }

  // Update master offset
  std::shared_ptr<SyncMasterDB> master_db =
      g_pika_rm->GetSyncMasterDBByName(DBInfo(db_name_));
  if (!master_db) {
    LOG(WARNING) << "Master DB: " << db_name_ << " not exist";
    slave_db->SetReplState(ReplState::kError);
    return false;
  }
  master_db->Logger()->SetProducerStatus(filenum, offset);
  slave_db->SetReplState(ReplState::kTryConnect);

  //now full sync is finished, remove unfinished full sync count
  g_pika_conf->RemoveInternalUsedUnfinishedFullSync(slave_db->DBName());

  return true;
}

void DB::PrepareRsync() {
  pstd::DeleteDirIfExist(dbsync_path_);
  int db_instance_num = g_pika_conf->db_instance_num();
  for (int index = 0; index < db_instance_num; index++) {
    pstd::CreatePath(dbsync_path_ + std::to_string(index));
  }
}

bool DB::IsBgSaving() {
  std::lock_guard ml(bgsave_protector_);
  return bgsave_info_.bgsaving;
}

/*
 * Change a new db locate in new_path
 * return true when change success
 * db remain the old one if return false
 */
bool DB::ChangeDb(const std::string& new_path) {
  std::string tmp_path(db_path_);
  if (tmp_path.back() == '/') {
    tmp_path.resize(tmp_path.size() - 1);
  }
  tmp_path += "_bak";
  pstd::DeleteDirIfExist(tmp_path);

  std::lock_guard l(dbs_rw_);
  LOG(INFO) << "DB: " << db_name_ << ", Prepare change db from: " << tmp_path;
  storage_.reset();

  if (0 != pstd::RenameFile(db_path_, tmp_path)) {
    LOG(WARNING) << "DB: " << db_name_
                 << ", Failed to rename db path when change db, error: " << strerror(errno);
    return false;
  }

  if (0 != pstd::RenameFile(new_path, db_path_)) {
    LOG(WARNING) << "DB: " << db_name_
                 << ", Failed to rename new db path when change db, error: " << strerror(errno);
    return false;
  }

  storage_ = std::make_shared<storage::Storage>(g_pika_conf->db_instance_num(),
      g_pika_conf->default_slot_num(), g_pika_conf->classic_mode());
  rocksdb::Status s = storage_->Open(g_pika_server->storage_options(), db_path_);
  assert(storage_);
  assert(s.ok());
  pstd::DeleteDirIfExist(tmp_path);
  LOG(INFO) << "DB: " << db_name_ << ", Change db success";
  return true;
}

void DB::ClearBgsave() {
  std::lock_guard l(bgsave_protector_);
  bgsave_info_.Clear();
}

void DB::UpdateCacheInfo(CacheInfo& cache_info) {
  std::unique_lock<std::shared_mutex> lock(cache_info_rwlock_);

  cache_info_.status = cache_info.status;
  cache_info_.cache_num = cache_info.cache_num;
  cache_info_.keys_num = cache_info.keys_num;
  cache_info_.used_memory = cache_info.used_memory;
  cache_info_.waitting_load_keys_num = cache_info.waitting_load_keys_num;
  cache_usage_ = cache_info.used_memory;

  uint64_t all_cmds = cache_info.hits + cache_info.misses;
  cache_info_.hitratio_all = (0 >= all_cmds) ? 0.0 : (cache_info.hits * 100.0) / all_cmds;

  uint64_t cur_time_us = pstd::NowMicros();
  uint64_t delta_time = cur_time_us - cache_info_.last_time_us + 1;
  uint64_t delta_hits = cache_info.hits - cache_info_.hits;
  cache_info_.hits_per_sec = delta_hits * 1000000 / delta_time;

  uint64_t delta_all_cmds = all_cmds - (cache_info_.hits + cache_info_.misses);
  cache_info_.read_cmd_per_sec = delta_all_cmds * 1000000 / delta_time;

  cache_info_.hitratio_per_sec = (0 >= delta_all_cmds) ? 0.0 : (delta_hits * 100.0) / delta_all_cmds;

  uint64_t delta_load_keys = cache_info.async_load_keys_num - cache_info_.last_load_keys_num;
  cache_info_.load_keys_per_sec = delta_load_keys * 1000000 / delta_time;

  cache_info_.hits = cache_info.hits;
  cache_info_.misses = cache_info.misses;
  cache_info_.last_time_us = cur_time_us;
  cache_info_.last_load_keys_num = cache_info.async_load_keys_num;
}

void DB::ResetDisplayCacheInfo(int status) {
  std::unique_lock<std::shared_mutex> lock(cache_info_rwlock_);
  cache_info_.status = status;
  cache_info_.cache_num = 0;
  cache_info_.keys_num = 0;
  cache_info_.used_memory = 0;
  cache_info_.hits = 0;
  cache_info_.misses = 0;
  cache_info_.hits_per_sec = 0;
  cache_info_.read_cmd_per_sec = 0;
  cache_info_.hitratio_per_sec = 0.0;
  cache_info_.hitratio_all = 0.0;
  cache_info_.load_keys_per_sec = 0;
  cache_info_.waitting_load_keys_num = 0;
  cache_usage_ = 0;
}
