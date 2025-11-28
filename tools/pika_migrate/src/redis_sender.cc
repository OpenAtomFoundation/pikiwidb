// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.


#include "include/redis_sender.h"

#include <time.h>
#include <unistd.h>

#include <glog/logging.h>

static time_t kCheckDiff = 1;
// 增加空闲 keepalive 周期（秒），应小于服务端 keepalive_timeout（如 60s），默认 30s
static time_t kKeepaliveInterval = 30;
static std::string kpingCmd = "*1\r\n$4\r\nPING\r\n";

RedisSender::RedisSender(int id, std::string ip, int64_t port, std::string user, std::string password):
  id_(id),
  cli_(NULL),
  ip_(ip),
  port_(port),
  user_(user),
  password_(password),
  should_exit_(false),
  elements_(0) {
  last_write_time_ = ::time(NULL);
}

RedisSender::~RedisSender() {
  LOG(INFO) << "RedisSender thread " << id_ << " exit!!!";
}

void RedisSender::ConnectRedis() {
  while (cli_ == NULL) {
    // Connect to redis
    cli_ = std::shared_ptr<net::NetCli>(net::NewRedisCli());
    cli_->set_connect_timeout(1000);
    cli_->set_recv_timeout(10000);
    cli_->set_send_timeout(10000);
    pstd::Status s = cli_->Connect(ip_, port_);
    if (!s.ok()) {
      LOG(WARNING) << "Can not connect to " << ip_ << ":" << port_ << ", status: " << s.ToString();
      cli_ = NULL;
      sleep(3);
      continue;
    } else {
      // Connect success
      // LOG(INFO) << "RedisSender thread " << id_ << "Connect to redis(" << ip_ << ":" << port_ << ") success";
      // Authentication
      if (!password_.empty()) {
        net::RedisCmdArgsType argv, resp;
        std::string cmd;

        argv.push_back("AUTH");
        argv.push_back(password_);
        net::SerializeRedisCommand(argv, &cmd);
        pstd::Status s = cli_->Send(&cmd);

        if (s.ok()) {
          s = cli_->Recv(&resp);
          if (resp[0] == "OK") {
          } else {
            LOG(FATAL) << "Connect to redis(" << ip_ << ":" << port_ << ") Invalid password";
            cli_->Close();
            cli_ = NULL;
            should_exit_ = true;
            return;
          }
        } else {
          LOG(WARNING) << "send auth failed: " << s.ToString();
          cli_->Close();
          cli_ = NULL;
          continue;
        }
      } else {
        // If forget to input password
        net::RedisCmdArgsType argv, resp;
        std::string cmd;

        argv.push_back("PING");
        net::SerializeRedisCommand(argv, &cmd);
        pstd::Status s = cli_->Send(&cmd);

        if (s.ok()) {
          s = cli_->Recv(&resp);
          if (s.ok()) {
            if (resp[0] == "NOAUTH Authentication required.") {
              LOG(FATAL) << "Ping redis(" << ip_ << ":" << port_ << ") NOAUTH Authentication required";
              cli_->Close();
              cli_ = NULL;
              should_exit_ = true;
              return;
            }
          } else {
            LOG(WARNING) << s.ToString();
            cli_->Close();
            cli_ = NULL;
          }
        }
      }
    }
  }
}

void RedisSender::Stop() {
  set_should_stop();
  should_exit_ = true;
  rsignal_.notify_all();
  wsignal_.notify_all();
}

void RedisSender::SendRedisCommand(const std::string &command) {
  std::unique_lock lock(signal_mutex_);
  wsignal_.wait(lock, [this]() { return commandQueueSize() < 100000; });
  if (!should_exit_) {
    std::lock_guard l(command_queue_mutex_);
    commands_queue_.push(command);
    rsignal_.notify_one();
  }
}

int RedisSender::SendCommand(std::string &command) {
  time_t now = ::time(NULL);
  if (kCheckDiff < now - last_write_time_) {
    int ret = cli_->CheckAliveness();
    if (ret < 0) {
      cli_ = nullptr;
      ConnectRedis();
    }
    last_write_time_ = now;
  }

  // Send command
  int idx = 0;
  do {
    pstd::Status s = cli_->Send(&command);

    if (s.ok()) {
      cli_->Recv(nullptr);
      return 0;
    }

    cli_->Close();
    cli_ = NULL;
    ConnectRedis();
  } while(++idx < 3);
  LOG(FATAL) << "RedisSender " << id_ << " fails to send redis command " << command << ", times: " << idx << ", error: " << "send command failed";
  return -1;
}

void *RedisSender::ThreadMain() {
  LOG(INFO) << "Start redis sender " << id_ << " thread...";
  int ret = 0;

  ConnectRedis();

  while (!should_exit_) {
    commands_mutex_.Lock();
    while (commands_queue_.size() == 0 && !should_exit_) {
      rsignal_.TimedWait(100);
      time_t whileNow = ::time(NULL);
      // 如果队列仍为空，定期保活（PING）
      if (commands_queue_.size() == 0 && cli_ != nullptr) {
        if (whileNow - last_write_time_ >= kKeepaliveInterval) {         
          int r = SendCommand(kpingCmd);
          if (r == 0) {
            LOG(INFO) << "RedisSender " << id_ << " keepalive PING sent to " << ip_ << ":" << port_;
            last_write_time_ = ::time(NULL);
          } else {
            LOG(WARNING) << "RedisSender " << id_ << " keepalive PING failed, will try reconnect";
          }
        }
      }
    }
    if (should_exit_) {
      break;
    }

    if (commandQueueSize() == 0) {
      continue;
    }

    // get redis command
    std::string command;
    commands_mutex_.Lock();
    command = commands_queue_.front();
    elements_++;
    commands_queue_.pop();
    wsignal_.Signal();
    commands_mutex_.Unlock();
    ret = SendCommand(command);
    if (ret == 0) {
      cnt_++;
    }

    wsignal_.notify_one();
    ret = SendCommand(command);

  }

  LOG(INFO) << "RedisSender thread " << id_ << " complete";
  cli_ = NULL;
  return NULL;
}
