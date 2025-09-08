#ifndef REDIS_SENDER_H_
#define REDIS_SENDER_H_

#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "net/include/net_thread.h"
#include "net/include/net_cli.h"
#include "net/include/redis_cli.h"

class RedisSender : public net::Thread {
 public:
  RedisSender(int id, std::string ip, int64_t port, std::string user, std::string password);
  virtual ~RedisSender();
  void Stop(void);
  int64_t elements() {
    return elements_;
  }

  void SendRedisCommand(const std::string &command);

 private:
  int SendCommand(std::string &command);
  void ConnectRedis();
  size_t commandQueueSize() {
    std::lock_guard l(command_queue_mutex_);
    return commands_queue_.size();
  }
  virtual void *ThreadMain();
 private:
  int id_;
  int port_;
  std::shared_ptr<net::NetCli> cli_;
  std::condition_variable rsignal_;
  std::condition_variable wsignal_;
  std::mutex signal_mutex_;
  std::mutex command_queue_mutex_;
  std::queue<std::string> commands_queue_;
  std::string ip_;
  std::string user_;
  std::string password_;
  bool should_exit_;
  int64_t elements_;
  std::atomic<time_t> last_write_time_;
};

#endif
