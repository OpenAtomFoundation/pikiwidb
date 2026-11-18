/*
 * Copyright (c) 2024-present, Qihoo, Inc. All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "praft/psnapshot.h"

#include <dirent.h>
#include <set>
#include <sys/stat.h>
#include <glog/logging.h>

#include "braft/local_file_meta.pb.h"
#include "butil/files/file_path.h"
#include "include/pika_conf.h"
#include "include/pika_server.h"
#include "praft/praft.h"
#include "storage/storage.h"
#include "storage/backupable.h"

extern std::unique_ptr<PikaConf> g_pika_conf;
extern std::unique_ptr<PikaServer> g_pika_server;

static bool IsDirectory(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}

static bool IsRegularFile(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }
  return S_ISREG(st.st_mode);
}

static std::string GetRelativePath(const std::string& full_path, const std::string& base_path) {
  if (full_path.find(base_path) == 0) {
    std::string relative = full_path.substr(base_path.length());
    if (!relative.empty() && relative[0] == '/') {
      relative = relative.substr(1);
    }
    return relative;
  }
  return full_path;
}

braft::FileAdaptor* PPosixFileSystemAdaptor::open(const std::string& path, int oflag,
                                                  const ::google::protobuf::Message* file_meta,
                                                  butil::File::Error* e) {
  if ((oflag & IS_RDONLY) == 0) {  // This is a read operation
    bool snapshots_exists = false;
    std::string snapshot_path;

    // parse snapshot path
    butil::FilePath parse_snapshot_path(path);
    std::vector<std::string> components;
    parse_snapshot_path.GetComponents(&components);
    for (const auto& component : components) {
      snapshot_path += component + "/";
      if (component.find("snapshot_") != std::string::npos) {
        break;
      }
    }

    // check whether snapshots have been created
    std::lock_guard<braft::raft_mutex_t> guard(mutex_);
    if (!snapshot_path.empty()) {
      DIR* dir = opendir(snapshot_path.c_str());
      if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
          std::string filename = entry->d_name;
          if (filename != "." && filename != ".." && filename.find(PRAFT_SNAPSHOT_META_FILE) == std::string::npos) {
            std::string full_path = snapshot_path + "/" + filename;
            if (IsRegularFile(full_path) || IsDirectory(full_path)) {
              // If the path directory contains files other than raft_snapshot_meta, snapshots have been generated
              snapshots_exists = true;
              break;
            }
          }
        }
        closedir(dir);
      }
    }

    // Snapshot generation
    if (!snapshots_exists && !snapshot_path.empty()) {
      braft::LocalSnapshotMetaTable snapshot_meta_memtable;
      std::string meta_path = snapshot_path + "/" PRAFT_SNAPSHOT_META_FILE;
      LOG(INFO) << "start to generate snapshot in path " << snapshot_path;
      braft::FileSystemAdaptor* fs = braft::default_file_system();
      assert(fs);
      snapshot_meta_memtable.load_from_file(fs, meta_path);

      if (g_pika_server) {
        auto db = g_pika_server->GetDB("db0");
        if (db) {
          std::set<std::string> dbs = {db->GetDBName()};
          TaskArg checkpoint_task(TaskType::kCreateCheckpoint, {snapshot_path});
          auto status = g_pika_server->DoSameThingSpecificDB(dbs, checkpoint_task);
          if (!status.ok()) {
            LOG(ERROR) << "Failed to create checkpoint for snapshot: " << status.ToString();
          }
        }
      }

      AddAllFiles(snapshot_path, &snapshot_meta_memtable, snapshot_path);

      // Update snapshot meta with last log index and term
      if (g_pika_server) {
        auto db = g_pika_server->GetDB("db0");
        if (db && db->storage()) {
          auto& new_meta = const_cast<braft::SnapshotMeta&>(snapshot_meta_memtable.meta());
          
          // Get the smallest flushed log index as the snapshot point
          uint64_t last_log_index = db->storage()->GetSmallestFlushedLogIndex();
          new_meta.set_last_included_index(last_log_index);
          
          // Get the term for this log index
          auto raft_mgr = g_pika_server->GetRaftManager();
          if (raft_mgr) {
            auto raft_node = raft_mgr->GetRaftNode("db0");
            if (raft_node && raft_node->GetRaftNode()) {
              braft::NodeStatus status;
              raft_node->GetRaftNode()->get_status(&status);
              new_meta.set_last_included_term(status.term);
              LOG(INFO) << "Updated snapshot meta: last_included_index=" << last_log_index 
                        << ", last_included_term=" << status.term;
            }
          }
        }
      }

      auto rc = snapshot_meta_memtable.save_to_file(fs, meta_path);
      if (rc == 0) {
        LOG(INFO) << "Succeed to save snapshot in path " << snapshot_path;
      } else {
        LOG(ERROR) << "Fail to save snapshot in path " << snapshot_path;
      }
      LOG(INFO) << "generate snapshot completed in path " << snapshot_path;
    }
  }

  return braft::PosixFileSystemAdaptor::open(path, oflag, file_meta, e);
}

void PPosixFileSystemAdaptor::AddAllFiles(const std::string& dir,
                                          braft::LocalSnapshotMetaTable* snapshot_meta_memtable,
                                          const std::string& base_path) {
  assert(snapshot_meta_memtable);
  DIR* dirp = opendir(dir.c_str());
  if (!dirp) {
    LOG(WARNING) << "Failed to open directory: " << dir;
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dirp)) != nullptr) {
    std::string filename = entry->d_name;
    if (filename == "." || filename == "..") {
      continue;
    }

    std::string full_path = dir + "/" + filename;
    if (IsDirectory(full_path)) {
      LOG(INFO) << "dir_path = " << full_path;
      AddAllFiles(full_path, snapshot_meta_memtable, base_path);
    } else if (IsRegularFile(full_path)) {
      std::string relative_path = GetRelativePath(full_path, base_path);
      LOG(INFO) << "file_path = " << relative_path;
      braft::LocalFileMeta meta;
      if (snapshot_meta_memtable->add_file(relative_path, meta) != 0) {
        LOG(WARNING) << "Failed to add file: " << relative_path;
      }
    }
  }
  closedir(dirp);
}
