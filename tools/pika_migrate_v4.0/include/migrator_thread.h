#ifndef MIGRATOR_THREAD_H_
#define MIGRATOR_THREAD_H_

#include <iostream>
#include <mutex>

#include "storage/storage.h"
#include "net/include/redis_cli.h"

#include "include/redis_sender.h"

class MigratorThread : public net::Thread {
 public:
  MigratorThread(std::shared_ptr<storage::Storage> storage_, std::vector<std::shared_ptr<RedisSender>>  *senders, int type, int thread_num) :
      storage_(storage_),
      should_exit_(false),
      senders_(senders),
      type_(type),
      thread_num_(thread_num),
      thread_index_(0),
      num_(0) {
  }

  virtual ~ MigratorThread();

  int64_t num() {
    std::lock_guard<std::mutex> l(num_mutex_);
    return num_;
  }

  void Stop() {
	  should_exit_ = true;
  }

 private:
  void PlusNum() {
    std::lock_guard<std::mutex> l(num_mutex_);
    ++num_;
  }

  void DispatchKey(const std::string &command, const std::string& key = "");

  void MigrateDB();
  void MigrateStringsDB();
  void MigrateListsDB();
  void MigrateHashesDB();
  void MigrateSetsDB();
  void MigrateZsetsDB();
  void MigrateStreamsDB();

  virtual void *ThreadMain();

 private:
  std::shared_ptr<storage::Storage> storage_;
  bool should_exit_;

  std::vector<std::shared_ptr<RedisSender>> *senders_;
  int type_;
  int thread_num_;
  int thread_index_;

  int64_t num_;
  std::mutex num_mutex_;
};

#endif
