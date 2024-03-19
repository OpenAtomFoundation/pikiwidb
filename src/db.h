//
// Created by dingxiaoshuai on 2024/3/18.
//

#ifndef PIKIWIDB_DB_H
#define PIKIWIDB_DB_H

#include <string>

#include "pstd/noncopyable.h"
#include "storage/storage.h"
#include "log.h"

namespace pikiwidb {
class DB : public pstd::noncopyable {
 public:
  DB(const int db_id, const std::string& db_path);
  std::unique_ptr<storage::Storage>& GetStorage() { return storage_; }

  std::shared_mutex& GetSharedMutex() { return storage_mutex_; }

 private:
  const int db_id_;
  const std::string db_path_;

  /**
   * If you want to change the pointer that points to storage,
   * you must first acquire a mutex lock.
   * If you only want to access the pointer,
   * you just need to obtain a shared lock.
   */
  std::shared_mutex storage_mutex_;
  bool opened_ = false;
  std::unique_ptr<storage::Storage> storage_;

  /**
   * If you want to change the status below，you must first acquire
   * a mutex lock.
   * If you only want to access the status below,
   * you just need to obtain a shared lock.
   */
  std::shared_mutex checkpoint_mutex_;
  bool checkpoint_in_process_ = false;
  int64_t last_checkpoint_time_ = -1;
  bool last_checkpoint_success_ = false;
};
}

#endif  // PIKIWIDB_DB_H
