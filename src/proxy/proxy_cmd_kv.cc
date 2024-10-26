#include "proxy_cmd_kv.h"
#include "base_cmd.h"
#include <memory>
#include <utility>
#include "pikiwidb.h"

// proxy_cmd_kv.cc
namespace pikiwidb {

std::string SetProxyCmd::GetCommand() {
  return "set " + key_ + " " + value_;
}

bool SetProxyCmd::DoInitial(PClient* client) {
 client_.reset(client);
 return true;
}
  
void SetProxyCmd::Execute() {
  // TODO: route, (leave interface for mget or mset,
  // split task and combine callback)
  // Codis: add sub task for mget or mset

  // route class: manage all brpc_redis
  // task -> route -> brpc_redis_
  // Commit might be launch from timer (better) or route
  // route::forward(cmd)

  ROUTER.forward(shared_from_this());
} 

void SetProxyCmd::CallBack() {
  client_->SetRes(CmdRes::kOK);
}

} // namespace pikiwidb

// List:
// 1. config file, set number of pikiwidb 
// 2. set flag and startup with proxy mode 
// 3. client sdk