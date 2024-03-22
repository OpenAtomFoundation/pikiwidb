//
// Created by dingxiaoshuai on 2024/3/20.
//

#ifndef PIKIWIDB_CHECKPOINT_MANAGER_H
#define PIKIWIDB_CHECKPOINT_MANAGER_H

#include <future>
#include <thread>
#include <utility>
#include <vector>
#include <shared_mutex>

#include "rocksdb/db.h"
#include "rocksdb/utilities/checkpoint.h"

namespace pikiwidb {

class DB;

// 有关这次快照生成之后的 Info ，之后要将这些信息写入到文件中，作为快照的一部分。
// 只写了部分信息，还需要什么信息，请 comment 。
struct CheckPointInfo {
  time_t start_checkpoint_time = 0;
  time_t finish_checkpoint_time = 0;
  bool checkpoint_in_process = false;
  bool last_checkpoint_success = false;
};

struct CheckpointEntry {
  std::string checkpoint_path;
  CheckPointInfo checkpoint_info;
};

class CheckpointManager {
 public:
  CheckpointManager() = default;
  ~CheckpointManager() = default;
  ;

  void Init(int instNum, const std::string& dump_dir, DB* db);

  void CreateCheckpoint();

  void WaitForCheckpointDone();

  bool CheckpointInProcess();

 private:
  int checkpoint_num_;
  std::string dump_parent_dir_;
  std::vector<std::future<void>> res_;
  DB* db_ = nullptr;

  std::shared_mutex shared_mutex_;
  std::vector<CheckpointEntry> checkpoint_entries_;
};
}  // namespace pikiwidb

#endif  // PIKIWIDB_CHECKPOINT_MANAGER_H
