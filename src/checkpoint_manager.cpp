//
// Created by dingxiaoshuai on 2024/3/20.
//

#include "checkpoint_manager.h"
#include "db.h"
#include "log.h"
#include "pstd/env.h"

namespace pikiwidb {

class DB;

void CheckpointManager::Init(int instNum, const std::string& dump_parent_dir, DB* db) {
  checkpoint_num_ = instNum;
  dump_parent_dir_ = dump_parent_dir;
  checkpoint_entries_.resize(checkpoint_num_);
  res_.reserve(checkpoint_num_);
  db_ = db;
  for (auto& checkpoint_entry : checkpoint_entries_) {
    checkpoint_entry.checkpoint_path = dump_parent_dir_ + '/' + std::to_string(db->GetDbIndex());
    INFO("checkpoint_entry.checkpoint_path = {}", checkpoint_entry.checkpoint_path);
  }
}

void CheckpointManager::CreateCheckpoint() {
  res_.clear();

  if (!pstd::FileExists(dump_parent_dir_)) {
    if (0 != pstd::CreatePath(dump_parent_dir_)) {
      WARN("Create Dir {} fail!", dump_parent_dir_);
      return;
    }
    INFO("Create Dir {} success!", dump_parent_dir_);
  }

  // 在这里统一将状态更改。
  std::lock_guard Lock(shared_mutex_);

  for (int i = 0; i < checkpoint_num_; ++i) {
    // 执行 checkpoint 的时候需要持有 db 中保护 storage 的共享锁，所以这里传入了 DB::DoBgSave
    // 生成 checkpoint 的调用不是很非常频繁，所以生成一个新的线程去做，没有使用线程池。
    checkpoint_entries_[i].checkpoint_info.checkpoint_in_process = true;
    checkpoint_entries_[i].checkpoint_info.start_checkpoint_time = time(nullptr);
    auto res = std::async(std::launch::async, &DB::DoBgSave, db_, std::ref(checkpoint_entries_[i].checkpoint_info),
                          std::ref(dump_parent_dir_), i);
    res_.push_back(std::move(res));
  }
}

// WaitForCheckpointDone 需要与 CreateCheckpoint 同步调用。
// 所以这里不再加锁。
void CheckpointManager::WaitForCheckpointDone() {
  for (auto& r : res_) {
    r.get();
  }
}

bool CheckpointManager::CheckpointInProcess() {
  std::shared_lock sharedLock(shared_mutex_);
  for (int i = 0; i < checkpoint_num_; ++i) {
    if (checkpoint_entries_[i].checkpoint_info.checkpoint_in_process) {
      return true;
    }
  }
  return false;
}

}  // namespace pikiwidb