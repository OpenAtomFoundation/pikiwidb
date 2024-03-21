//
// Created by dingxiaoshuai on 2024/3/18.
//

#ifndef PIKIWIDB_DB_H
#define PIKIWIDB_DB_H

#include <string>

#include "checkpoint_manager.h"
#include "log.h"
#include "pstd/noncopyable.h"
#include "storage/storage.h"
namespace pikiwidb {

struct CheckPointContext {
  bool checkpoint_in_process = false;
  time_t last_checkpoint_time = 0;
  bool last_checkpoint_success = false;
  std::string checkpoint_path;
};

class DB {
 public:
  DB(int db_index, const std::string& db_path, const std::string& dump_path);
  std::unique_ptr<storage::Storage>& GetStorage() { return storage_; }

  void Lock() { storage_mutex_.lock(); }

  void UnLock() { storage_mutex_.unlock(); }

  void LockShared() { storage_mutex_.lock_shared(); }

  void UnLockShared() { storage_mutex_.unlock_shared(); }

  void CreateCheckpoint();

  [[maybe_unused]] void DoBgSave(CheckPointInfo&, const std::string&, int i);

  void FinishCheckpointDone(bool sync);

  int GetDbIndex() { return db_index_; }

 private:
  const int db_index_;
  const std::string db_path_;
  const std::string dump_parent_path_;
  const std::string dump_path_;
  /**
   * If you want to change the pointer that points to storage,
   * you must first acquire a mutex lock.
   * If you only want to access the pointer,
   * you just need to obtain a shared lock.
   */
  std::shared_mutex storage_mutex_;
  std::unique_ptr<storage::Storage> storage_;
  bool opened_ = false;

  /**
   * If you want to change the status below，you must first acquire
   * a mutex lock.
   * If you only want to access the status below,
   * you just need to obtain a shared lock.
   */
  std::shared_mutex checkpoint_mutex_;
  bool checkpoint_in_process = false;
  std::unique_ptr<CheckpointManager> checkpoint_manager_;
};
}  // namespace pikiwidb

#endif  // PIKIWIDB_DB_H
