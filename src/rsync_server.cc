// Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <filesystem>

#include <glog/logging.h>
#include <google/protobuf/map.h>

#include "pstd_hash.h"
#include "include/pika_server.h"
#include "include/rsync_server.h"
#include "pstd/include/pstd_defer.h"

extern PikaServer* g_pika_server;
namespace rsync {

using namespace net;
using namespace pstd;
using namespace RsyncService;

void RsyncWriteResp(RsyncService::RsyncResponse& response, std::shared_ptr<net::PbConn> conn) {
  std::string reply_str;
  if (!response.SerializeToString(&reply_str) || (conn->WriteResp(reply_str) != 0)) {
    LOG(WARNING) << "Process FileRsync request serialization failed";
    conn->NotifyClose();
    return;
  }
  conn->NotifyWrite();
}

RsyncServer::RsyncServer(const std::set<std::string>& ips, const int port) {
  work_thread_ = std::make_unique<net::ThreadPool>(2, 100000, "RsyncServerWork");
  rsync_server_thread_ = std::make_unique<RsyncServerThread>(ips, port, 1 * 1000, this);
}

RsyncServer::~RsyncServer() {
  //TODO: handle destory
  LOG(INFO) << "Rsync server destroyed";
}

void RsyncServer::Schedule(net::TaskFunc func, void* arg) {
  work_thread_->Schedule(func, arg);
}

int RsyncServer::Start() {
  LOG(INFO) << "start RsyncServer ...";
  rsync_server_thread_->set_thread_name("RsyncServerThread");
  int res = rsync_server_thread_->StartThread();
  if (res != net::kSuccess) {
    LOG(FATAL) << "Start rsync Server Thread Error. ret_code: " << res << " message: "
               << (res == net::kBindError ? ": bind port conflict" : ": other error");
  }
  res = work_thread_->start_thread_pool();
  if (res != net::kSuccess) {
    LOG(FATAL) << "Start rsync Server ThreadPool Error, ret_code: " << res << " message: "
               << (res == net::kCreateThreadError ? ": create thread error " : ": other error");
  }
  LOG(INFO) << "RsyncServer started ...";
  return res;
}

int RsyncServer::Stop() {
  LOG(INFO) << "stop RsyncServer ...";
  work_thread_->stop_thread_pool();
  rsync_server_thread_->StopThread();
  return 0;
}

RsyncServerConn::RsyncServerConn(int connfd, const std::string& ip_port, Thread* thread,
                                 void* worker_specific_data, NetMultiplexer* mpx)
    : PbConn(connfd, ip_port, thread, mpx), data_(worker_specific_data) {
  readers_.resize(kMaxRsyncParallelNum);
  for (int i = 0; i < kMaxRsyncParallelNum; i++) {
    readers_[i].reset(new RsyncReader());
  }
}

RsyncServerConn::~RsyncServerConn() {
  {
    std::lock_guard<std::mutex> guard(mu_);
    for (int i = 0; i < readers_.size(); i++) {
      readers_[i].reset();
    }
  }
  // Release dump ownership when connection closes (Scheme A)
  if (!snapshot_uuid_.empty()) {
    LOG(INFO) << "[RsyncServerConn] Connection " << conn_id_ << " closing, releasing dump " << snapshot_uuid_;
    g_pika_server->ReleaseDump(snapshot_uuid_);
  }
  // Unregister snapshot when connection closes (outside of mu_ lock)
  UnregisterSnapshot();
}

void RsyncServerConn::RegisterSnapshot(const std::string& snapshot_uuid) {
  if (!snapshot_uuid.empty() && snapshot_uuid_ != snapshot_uuid) {
    // Unregister old snapshot if different
    if (!snapshot_uuid_.empty()) {
      UnregisterSnapshot();
    }
    snapshot_uuid_ = snapshot_uuid;
    g_pika_server->RegisterRsyncSnapshot(snapshot_uuid_);
  }
}

void RsyncServerConn::UnregisterSnapshot() {
  if (!snapshot_uuid_.empty()) {
    // Clear any remaining transferring files
    std::set<std::string> remaining_files;
    {
      std::lock_guard<std::mutex> guard(mu_);
      remaining_files = transferring_files_;
      transferring_files_.clear();
    }
    for (const auto& file : remaining_files) {
      g_pika_server->UnregisterRsyncTransferringFile(snapshot_uuid_, file);
    }
    g_pika_server->UnregisterRsyncSnapshot(snapshot_uuid_);
    snapshot_uuid_.clear();
  }
}

void RsyncServerConn::AddTransferringFile(const std::string& filename) {
  if (!snapshot_uuid_.empty() && !filename.empty()) {
    std::lock_guard<std::mutex> guard(mu_);
    transferring_files_.insert(filename);
    g_pika_server->RegisterRsyncTransferringFile(snapshot_uuid_, filename);
  }
}

void RsyncServerConn::RemoveTransferringFile(const std::string& filename, bool is_eof) {
  if (!snapshot_uuid_.empty() && !filename.empty()) {
    std::lock_guard<std::mutex> guard(mu_);
    transferring_files_.erase(filename);
    g_pika_server->UnregisterRsyncTransferringFile(snapshot_uuid_, filename);

    // Only process cleanup when file transfer is complete (is_eof=true)
    if (is_eof) {
      std::string dump_path = g_pika_server->GetDumpPathBySnapshot(snapshot_uuid_);
      std::string filepath = dump_path + "/" + filename;

      // Check if file is orphan (nlink=1, only referenced by dump, not by db)
      struct stat st;
      if (stat(filepath.c_str(), &st) == 0 && st.st_nlink == 1) {
        // Orphan file: schedule for delayed cleanup (10 minutes)
        // This allows Slave to retry if needed before actual deletion
        g_pika_server->ScheduleFileForCleanup(filepath, 600);
        LOG(INFO) << "[RsyncTransfer] Scheduled orphan file for cleanup: " << filename
                  << " for snapshot: " << snapshot_uuid_;
      }
      // Non-orphan files (nlink=2) are still referenced by RocksDB, no cleanup needed
    }
  }
}

bool RsyncServerConn::IsFileTransferring(const std::string& filename) const {
  std::lock_guard<std::mutex> guard(mu_);
  return transferring_files_.find(filename) != transferring_files_.end();
}

std::set<std::string> RsyncServerConn::GetTransferringFiles() const {
  std::lock_guard<std::mutex> guard(mu_);
  return transferring_files_;
}

bool RsyncServerConn::IsFileTransferringGlobally(const std::string& snapshot_uuid, const std::string& filename) {
  if (g_pika_server) {
    return g_pika_server->IsRsyncFileTransferring(snapshot_uuid, filename);
  }
  return false;
}

int RsyncServerConn::DealMessage() {
  std::shared_ptr<RsyncService::RsyncRequest> req = std::make_shared<RsyncService::RsyncRequest>();
  bool parse_res = req->ParseFromArray(rbuf_ + cur_pos_ - header_len_, header_len_);
  if (!parse_res) {
    LOG(WARNING) << "Pika rsync server connection pb parse error.";
    return -1;
  }
  switch (req->type()) {
    case RsyncService::kRsyncMeta: {
      auto task_arg =
          new RsyncServerTaskArg(req, std::dynamic_pointer_cast<RsyncServerConn>(shared_from_this()));
          ((RsyncServer*)(data_))->Schedule(&RsyncServerConn::HandleMetaRsyncRequest, task_arg);
          break;
      }
      case RsyncService::kRsyncFile: {
        auto task_arg =
            new RsyncServerTaskArg(req, std::dynamic_pointer_cast<RsyncServerConn>(shared_from_this()));
            ((RsyncServer*)(data_))->Schedule(&RsyncServerConn::HandleFileRsyncRequest, task_arg);
            break;
      }
      default: {
        LOG(WARNING) << "Invalid RsyncRequest type";
      }
    }
    return 0;
}

void RsyncServerConn::HandleMetaRsyncRequest(void* arg) {
  std::unique_ptr<RsyncServerTaskArg> task_arg(static_cast<RsyncServerTaskArg*>(arg));
  const std::shared_ptr<RsyncService::RsyncRequest> req = task_arg->req;
  std::shared_ptr<net::PbConn> conn = task_arg->conn;
  std::string db_name = req->db_name();
  std::shared_ptr<DB> db = g_pika_server->GetDB(db_name);

  RsyncService::RsyncResponse response;
  response.set_reader_index(req->reader_index());
  response.set_code(RsyncService::kOk);
  response.set_type(RsyncService::kRsyncMeta);
  response.set_db_name(db_name);
  /*
   * Since the slot field is written in protobuffer,
   * slot_id is set to the default value 0 for compatibility
   * with older versions, but slot_id is not used
   */
  response.set_slot_id(0);

  std::string snapshot_uuid;
  if (!db || db->IsBgSaving()) {
    LOG(WARNING) << "waiting bgsave done...";
    response.set_snapshot_uuid(snapshot_uuid);
    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  std::vector<std::string> filenames;
  g_pika_server->GetDumpMeta(db_name, &filenames, &snapshot_uuid);
  response.set_snapshot_uuid(snapshot_uuid);

  // Get db_ptr early for use in checks
  std::shared_ptr<DB> db_ptr = g_pika_server->GetDB(db_name);

  // Check if dump directory exists and has files
  if (filenames.empty()) {
    LOG(ERROR) << "[Rsync Meta] No files found in dump directory for db: " << db_name
               << ", path: " << (db_ptr ? db_ptr->bgsave_info().path : "unknown")
               << ". Triggering new bgsave.";
    db->BgSaveDB();
    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  // Check if current dump directory has all required files (integrity check)
  // If files are missing, the dump is corrupted and needs to be regenerated
  if (db_ptr) {
    const std::string dump_path = db_ptr->bgsave_info().path;
    std::vector<std::string> missing_files;
    for (const auto& filename : filenames) {
      std::string full_path = dump_path + "/" + filename;
      if (!pstd::FileExists(full_path)) {
        missing_files.push_back(filename);
      }
    }

    if (!missing_files.empty()) {
      LOG(ERROR) << "[Rsync Meta] Dump integrity check failed for snapshot: " << snapshot_uuid
                 << ", missing " << missing_files.size() << " files"
                 << ", first missing: " << (missing_files.empty() ? "none" : missing_files[0])
                 << ". Deleting and triggering new bgsave.";

      // Delete the corrupted dump
      pstd::DeleteDirIfExist(dump_path);

      // Trigger new bgsave
      db->BgSaveDB();

      response.set_code(RsyncService::kErr);
      RsyncWriteResp(response, conn);
      return;
    }
  }

  // Get connection and dump path
  auto conn_ptr = std::dynamic_pointer_cast<RsyncServerConn>(conn);
  std::string dump_path = db_ptr ? db_ptr->bgsave_info().path : "";

  // Integrity Check: Re-scan dump directory to verify file consistency
  // This detects files that were deleted between GetDumpMeta and now
  if (!dump_path.empty()) {
    std::vector<std::string> actual_files;
    // Scan each instance directory (0, 1, 2, etc.)
    std::vector<std::string> subDirs;
    if (pstd::GetChildren(dump_path, subDirs) == 0) {
      for (const auto& subDir : subDirs) {
        std::string instPath = dump_path + "/" + subDir;
        if (!pstd::FileExists(instPath) || pstd::IsDir(instPath) != 0) {
          continue;
        }
        std::vector<std::string> instFiles;
        if (pstd::GetChildren(instPath, instFiles) == 0) {
          for (const auto& file : instFiles) {
            actual_files.push_back(subDir + "/" + file);
          }
        }
      }
    }

    // Compare filenames (from GetDumpMeta) with actual_files (re-scanned)
    std::vector<std::string> missing_files;
    for (const auto& expected : filenames) {
      bool found = false;
      for (const auto& actual : actual_files) {
        if (actual == expected) {
          found = true;
          break;
        }
      }
      if (!found && expected != "info") {  // info file is handled separately
        missing_files.push_back(expected);
      }
    }

    if (!missing_files.empty()) {
      LOG(ERROR) << "[Rsync Meta] Dump integrity check failed for db: " << db_name
                 << ", missing " << missing_files.size() << " files"
                 << ", first missing: " << missing_files[0]
                 << ", deleting dump and triggering new bgsave";
      LOG(ERROR) << "[Integrity Check] Missing files in " << dump_path << ": "
                 << pstd::StringConcat(missing_files, ',');

      pstd::DeleteDirIfExist(dump_path);
      db->BgSaveDB();

      response.set_code(RsyncService::kErr);
      RsyncWriteResp(response, conn);
      return;
    }
  }

  // Get connection ID for dump ownership tracking (use connection object address)
  std::string conn_id = conn_ptr ? std::to_string(reinterpret_cast<uint64_t>(conn_ptr.get())) : "";

  // Check if this dump is already in use by another slave
  // Scheme A: Each slave has exclusive dump ownership
  if (g_pika_server->IsDumpInUse(snapshot_uuid)) {
    LOG(INFO) << "[Rsync Meta] Dump " << snapshot_uuid << " is already in use by another slave."
              << " Active dumps: " << g_pika_server->GetActiveDumpCount()
              << "/" << PikaServer::kMaxConcurrentDumps
              << ". Triggering new bgsave.";

    // Trigger new bgsave for this slave
    db->BgSaveDB();

    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  // Check concurrent dump limit
  if (g_pika_server->GetActiveDumpCount() >= PikaServer::kMaxConcurrentDumps) {
    LOG(WARNING) << "[Rsync Meta] Max concurrent dumps (" << PikaServer::kMaxConcurrentDumps
                 << ") reached. Rejecting new sync request.";

    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  // Mark this dump as in use by this connection
  if (conn_ptr && !dump_path.empty()) {
    if (!g_pika_server->MarkDumpInUse(snapshot_uuid, conn_id, dump_path)) {
      LOG(WARNING) << "[Rsync Meta] Failed to mark dump " << snapshot_uuid << " in use."
                   << " Possibly concurrent access. Triggering new bgsave.";

      db->BgSaveDB();

      response.set_code(RsyncService::kErr);
      RsyncWriteResp(response, conn);
      return;
    }

    // Store connection ID in the connection object
    conn_ptr->conn_id_ = conn_id;

    // Register snapshot for tracking
    // Note: RegisterSnapshot will set snapshot_uuid_ internally, don't set it here
    conn_ptr->RegisterSnapshot(snapshot_uuid);

    // Pre-register all files for protection during transfer
    for (const auto& filename : filenames) {
      conn_ptr->AddTransferringFile(filename);
    }

    LOG(INFO) << "[Rsync Meta] Dump " << snapshot_uuid << " reserved for connection " << conn_id
              << ", files count: " << filenames.size();
  }

  LOG(INFO) << "Rsync Meta request, snapshot_uuid: " << snapshot_uuid
            << " files count: " << filenames.size()
            << " active dumps: " << g_pika_server->GetActiveDumpCount()
            << "/" << PikaServer::kMaxConcurrentDumps;

  std::for_each(filenames.begin(), filenames.end(), [](auto& file) {
    LOG(INFO) << "rsync snapshot file: " << file;
  });

  RsyncService::MetaResponse* meta_resp = response.mutable_meta_resp();
  for (const auto& filename : filenames) {
    meta_resp->add_filenames(filename);
  }
  RsyncWriteResp(response, conn);
}

void RsyncServerConn::HandleFileRsyncRequest(void* arg) {
  std::unique_ptr<RsyncServerTaskArg> task_arg(static_cast<RsyncServerTaskArg*>(arg));
  const std::shared_ptr<RsyncService::RsyncRequest> req = task_arg->req;
  std::shared_ptr<RsyncServerConn> conn = task_arg->conn;

  std::string db_name = req->db_name();
  std::string filename = req->file_req().filename();
  size_t offset = req->file_req().offset();
  size_t count = req->file_req().count();

  RsyncService::RsyncResponse response;
  response.set_reader_index(req->reader_index());
  response.set_code(RsyncService::kOk);
  response.set_type(RsyncService::kRsyncFile);
  response.set_db_name(db_name);
  /*
   * Since the slot field is written in protobuffer,
   * slot_id is set to the default value 0 for compatibility
   * with older versions, but slot_id is not used
   */
  response.set_slot_id(0);

  std::string snapshot_uuid;
  Status s = g_pika_server->GetDumpUUID(db_name, &snapshot_uuid);
  response.set_snapshot_uuid(snapshot_uuid);
  if (!s.ok()) {
    LOG(WARNING) << "rsyncserver get snapshotUUID failed";
    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  std::shared_ptr<DB> db = g_pika_server->GetDB(db_name);
  if (!db) {
   LOG(WARNING) << "cannot find db for db_name: " << db_name;
   response.set_code(RsyncService::kErr);
   RsyncWriteResp(response, conn);
   return;
  }

  const std::string filepath = db->bgsave_info().path + "/" + filename;

  // Check if file exists (may have been cleaned up during sync)
  // If file doesn't exist, return error to let Slave retry and potentially trigger new bgsave
  if (!pstd::FileExists(filepath)) {
    LOG(WARNING) << "File no longer exists, returning error: " << filepath;
    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    return;
  }

  // Register this file as being transferred
  // This prevents the file from being cleaned up during orphan file cleanup
  conn->AddTransferringFile(filename);

  char* buffer = new char[req->file_req().count() + 1];
  size_t bytes_read{0};
  std::string checksum = "";
  bool is_eof = false;
  std::shared_ptr<RsyncReader> reader = conn->readers_[req->reader_index()];
  s = reader->Read(filepath, offset, count, buffer,
                   &bytes_read, &checksum, &is_eof);

  // Unregister this file after transfer (whether successful or not)
  // Only cleanup the file if transfer is complete (is_eof=true)
  conn->RemoveTransferringFile(filename, is_eof);

  if (!s.ok()) {
    response.set_code(RsyncService::kErr);
    RsyncWriteResp(response, conn);
    delete []buffer;
    return;
  }

  RsyncService::FileResponse* file_resp = response.mutable_file_resp();
  file_resp->set_data(buffer, bytes_read);
  file_resp->set_eof(is_eof);
  file_resp->set_checksum(checksum);
  file_resp->set_filename(filename);
  file_resp->set_count(bytes_read);
  file_resp->set_offset(offset);

  RsyncWriteResp(response, conn);
  delete []buffer;
}

RsyncServerThread::RsyncServerThread(const std::set<std::string>& ips, int port, int cron_interval, RsyncServer* arg)
    : HolyThread(ips, port, &conn_factory_, cron_interval, &handle_, true), conn_factory_(arg) {}

RsyncServerThread::~RsyncServerThread() {
  LOG(WARNING) << "RsyncServerThread destroyed";
}

void RsyncServerThread::RsyncServerHandle::FdClosedHandle(int fd, const std::string& ip_port) const {
  LOG(WARNING) << "ip_port: " << ip_port << " connection closed";
}

void RsyncServerThread::RsyncServerHandle::FdTimeoutHandle(int fd, const std::string& ip_port) const {
  LOG(WARNING) << "ip_port: " << ip_port << " connection timeout";
}

bool RsyncServerThread::RsyncServerHandle::AccessHandle(int fd, std::string& ip_port) const {
  LOG(WARNING) << "fd: "<< fd << " ip_port: " << ip_port << " connection accepted";
  return true;
}

void RsyncServerThread::RsyncServerHandle::CronHandle() const {
}

} // end namespace rsync

