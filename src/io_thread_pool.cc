/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <cassert>

#include "io_thread_pool.h"
#include "pstd/log.h"
#include "util.h"

namespace pikiwidb {

const size_t IOThreadPool::kMaxWorkers = 128;

bool IOThreadPool::SetWorkerNum(size_t num) {
  if (num <= 1) {
    return true;
  }

  if (state_ != State::kNone) {
    ERROR("can only called before application run");
    return false;
  }

  if (!worker_loops_.empty()) {
    ERROR("can only called once, not empty loops size: {}", worker_loops_.size());
    return false;
  }

  worker_num_.store(num);
  worker_threads_.reserve(num);
  worker_loops_.reserve(num);

  return true;
}

bool IOThreadPool::Init(const char* ip, int port, const NewTcpConnectionCallback& cb) {
  auto f = [this] { return ChooseNextWorkerEventLoop(); };

  base_.Init();
  INFO("base loop {} {}, g_baseLoop {}", base_.GetName(), static_cast<void*>(&base_),
       static_cast<void*>(pikiwidb::EventLoop::Self()));
  if (!base_.Listen(ip, port, cb, f)) {
    ERROR("can not bind socket on addr {}:{}", ip, port);
    return false;
  }

  return true;
}

void IOThreadPool::Run(int ac, char* av[]) {
  assert(state_ == State::kNone);
  INFO("Process {} starting...", name_);

  // start loops in thread pool
  StartWorkers();
  base_.Run();

  for (auto& w : worker_threads_) {
    if (w.joinable()) {
      w.join();
    }
  }

  worker_threads_.clear();

  INFO("Process {} stopped, goodbye...", name_);
}

void IOThreadPool::Exit() {
  state_ = State::kStopped;

  BaseLoop()->Stop();
  for (const auto& worker_loop : worker_loops_) {
    EventLoop* loop = worker_loop.get();
    loop->Stop();
  }
}

bool IOThreadPool::IsExit() const { return state_ == State::kStopped; }

EventLoop* IOThreadPool::BaseLoop() { return &base_; }

EventLoop* IOThreadPool::ChooseNextWorkerEventLoop() {
  if (worker_loops_.empty()) {
    return BaseLoop();
  }

  auto& loop = worker_loops_[current_worker_loop_++ % worker_loops_.size()];
  return loop.get();
}

void IOThreadPool::StartWorkers() {
  // only called by main thread
  assert(state_ == State::kNone);

  size_t index = 1;
  while (worker_loops_.size() < worker_num_) {
    std::unique_ptr<EventLoop> loop = std::make_unique<EventLoop>();
    if (!name_.empty()) {
      loop->SetName(name_ + "_" + std::to_string(index++));
      INFO("loop {}, name {}", static_cast<void*>(loop.get()), loop->GetName().c_str());
    }
    worker_loops_.push_back(std::move(loop));
  }

  for (index = 0; index < worker_loops_.size(); ++index) {
    EventLoop* loop = worker_loops_[index].get();
    std::thread t([loop]() {
      loop->Init();
      loop->Run();
    });
    INFO("thread {}, thread loop {}, loop name {}", index, static_cast<void*>(loop), loop->GetName().c_str());
    worker_threads_.push_back(std::move(t));
  }

  state_ = State::kStarted;
}

void IOThreadPool::SetName(const std::string& name) { name_ = name; }

bool IOThreadPool::Listen(const char* ip, int port, const NewTcpConnectionCallback& ccb) {
  auto f = [this] { return ChooseNextWorkerEventLoop(); };
  auto loop = BaseLoop();
  return loop->Execute([loop, ip, port, ccb, f]() { return loop->Listen(ip, port, ccb, f); }).get();
}

void IOThreadPool::Connect(const char* ip, int port, const NewTcpConnectionCallback& ccb,
                           const TcpConnectionFailCallback& fcb, EventLoop* loop) {
  if (!loop) {
    loop = ChooseNextWorkerEventLoop();
  }

  std::string ipStr(ip);
  loop->Execute([loop, ipStr, port, ccb, fcb]() { loop->Connect(ipStr.c_str(), port, ccb, fcb); });
}

std::shared_ptr<HttpServer> IOThreadPool::ListenHTTP(const char* ip, int port, HttpServer::OnNewClient cb) {
  auto server = std::make_shared<HttpServer>();
  server->SetOnNewHttpContext(std::move(cb));

  // capture server to make it long live with TcpListener
  auto ncb = [server](TcpConnection* conn) { server->OnNewConnection(conn); };
  Listen(ip, port, ncb);

  return server;
}

std::shared_ptr<HttpClient> IOThreadPool::ConnectHTTP(const char* ip, int port, EventLoop* loop) {
  auto client = std::make_shared<HttpClient>();

  // capture client to make it long live with TcpConnection
  auto ncb = [client](TcpConnection* conn) { client->OnConnect(conn); };
  auto fcb = [client](EventLoop*, const char* ip, int port) { client->OnConnectFail(ip, port); };

  if (!loop) {
    loop = ChooseNextWorkerEventLoop();
  }
  client->SetLoop(loop);
  Connect(ip, port, std::move(ncb), std::move(fcb), loop);

  return client;
}

void IOThreadPool::Reset() {
  state_ = State::kNone;
  BaseLoop()->Reset();
}

void WorkIOThreadPool::PushWriteTask(std::shared_ptr<PClient> client) {
  auto pos = ++counter_ % worker_num_;
  std::unique_lock lock(*writeMutex_[pos]);

  writeQueue_[pos].emplace_back(client);
  writeCond_[pos]->notify_one();
}

void WorkIOThreadPool::StartWorkers() {
  // only called by main thread
  assert(state_ == State::kNone);

  IOThreadPool::StartWorkers();

  writeMutex_.reserve(worker_num_);
  writeCond_.reserve(worker_num_);
  writeQueue_.reserve(worker_num_);
  for (size_t index = 0; index < worker_num_; ++index) {
    writeMutex_.emplace_back(std::make_unique<std::mutex>());
    writeCond_.emplace_back(std::make_unique<std::condition_variable>());
    writeQueue_.emplace_back();

    std::thread t([this, index]() {
      while (writeRunning_) {
        std::unique_lock lock(*writeMutex_[index]);
        while (writeQueue_[index].empty()) {
          if (!writeRunning_) {
            break;
          }
          writeCond_[index]->wait(lock);
        }
        if (!writeRunning_) {
          break;
        }
        auto client = writeQueue_[index].front();
        if (client->State() == ClientState::kOK) {
          client->WriteReply2Client();
        }
        writeQueue_[index].pop_front();
      }
      INFO("worker write thread {}, goodbye...", index);
    });

    INFO("worker write thread {}, starting...", index);
    writeThreads_.push_back(std::move(t));
  }
}

void WorkIOThreadPool::Exit() {
  IOThreadPool::Exit();

  writeRunning_ = false;
  int i = 0;
  for (auto& cond : writeCond_) {
    std::unique_lock lock(*writeMutex_[i++]);
    cond->notify_all();
  }
  for (auto& wt : writeThreads_) {
    if (wt.joinable()) {
      wt.join();
    }
  }
  writeThreads_.clear();
  writeCond_.clear();
  writeQueue_.clear();
  writeMutex_.clear();
}

}  // namespace pikiwidb
