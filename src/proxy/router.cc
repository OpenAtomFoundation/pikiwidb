#include "router.h"
#include "config.h"
#include <functional>
#include <vector>
namespace pikiwidb {

extern PConfig g_config;

Router::~Router() { 
  timer_wheel_->Stop();
  INFO("ROUTER is closing..."); 
}

Router& Router::Instance() {
  static Router router;
  return router;
}

void Router::Init() {
  // TODO: config.cc main()...
  // TODO: register brpc_redis from cfg_file_
  std::vector<PString> ip_group = SplitString(g_config.proxy_ip.ToString(), ';');
  
  for (auto &ip : ip_group) {
    std::unique_ptr<BrpcRedis> brpc_redis_ptr(new BrpcRedis());
    if (brpc_redis_ptr->Open(ip) != 0) {
      LOG(ERROR) << "Fail to init channel to pikiwidb, ip " << ip; 
      continue;
    }
    brpc_redis_.push_back(std::move(brpc_redis_ptr));
  }

  brpc_redis_num_ = brpc_redis_.size();
  timer_wheel_ = new TimerWheel(10, 1000);
  for (auto &brpc : brpc_redis_) {
    timer_wheel_->AddTask(1000, [&]() { brpc->Commit(); }); 
  }
  timer_wheel_->Start();
}

void Router::forward(std::shared_ptr<ProxyBaseCmd> task) {
  // TODO (Tangruilin): add pd 
  // hash key 
  brpc_redis_[hasher_(task->GetKey()) % brpc_redis_num_]->PushRedisTask(task); 
}
  
}