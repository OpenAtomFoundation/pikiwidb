#pragma once
#include <brpc/channel.h>
#include <brpc/redis.h>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "brpc/redis_reply.h"
#include "storage/src/redis.h"
#include "proxy_base_cmd.h"

namespace pikiwidb {

class BrpcRedis {
public:
  void Init() { options.protocol = brpc::PROTOCOL_REDIS; }
  
  int Open(PString &ip);
  
  void PushRedisTask(const std::shared_ptr<ProxyBaseCmd>& task);

  void Commit();

  // brpc::Channel GetChannel() { return channel_; }
  
  // brpc::ChannelOptions GetOptions() { return options; }

  BrpcRedis() { this->Init(); }
  
private:
  void SetResponse(const brpc::RedisResponse& resp, const std::shared_ptr<ProxyBaseCmd>& task, size_t index);

  brpc::Channel channel_;
  brpc::ChannelOptions options;
  std::mutex lock__;
  std::vector<std::shared_ptr<ProxyBaseCmd>> tasks_;
  size_t batch_size_ = 5; 
};
}

