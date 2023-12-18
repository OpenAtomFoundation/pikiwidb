/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "command.h"
#include "replication.h"

using std::size_t;

namespace pikiwidb {

const PCommandInfo PCommandTable::s_info[] = {
    // key
    {"type", kPAttrRead, 2, &type},
    {"exists", kPAttrRead, 2, &exists},
    {"del", kPAttrWrite, -2, &del},
    {"expire", kPAttrRead, 3, &expire},
    {"ttl", kPAttrRead, 2, &ttl},
    {"pexpire", kPAttrRead, 3, &pexpire},
    {"pttl", kPAttrRead, 2, &pttl},
    {"expireat", kPAttrRead, 3, &expireat},
    {"pexpireat", kPAttrRead, 3, &pexpireat},
    {"persist", kPAttrRead, 2, &persist},
    {"move", kPAttrWrite, 3, &move},
    {"keys", kPAttrRead, 2, &keys},
    {"randomkey", kPAttrRead, 1, &randomkey},
    {"rename", kPAttrWrite, 3, &rename},
    {"renamenx", kPAttrWrite, 3, &renamenx},
    {"scan", kPAttrRead, -2, &scan},
    {"sort", kPAttrRead, -2, &sort},

    // server
    {"select", kPAttrRead, 2, &select},
    {"dbsize", kPAttrRead, 1, &dbsize},
    {"bgsave", kPAttrRead, 1, &bgsave},
    {"save", kPAttrRead, 1, &save},
    {"lastsave", kPAttrRead, 1, &lastsave},
    {"flushdb", kPAttrWrite, 1, &flushdb},
    {"flushall", kPAttrWrite, 1, &flushall},
    {"client", kPAttrRead, -2, &client},
    {"debug", kPAttrRead, -2, &debug},
    {"shutdown", kPAttrRead, -1, &shutdown},
    {"ping", kPAttrRead, 1, &ping},
    {"echo", kPAttrRead, 2, &echo},
    {"info", kPAttrRead, -1, &info},
    {"monitor", kPAttrRead, 1, &monitor},
    {"auth", kPAttrRead, 2, &auth},
    {"slowlog", kPAttrRead, -2, &slowlog},
    // {"config", PAttr_read, -3, &config},

    // string
    {"strlen", kPAttrRead, 2, &strlen},
    {"mset", kPAttrWrite, -3, &mset},
    {"msetnx", kPAttrWrite, -3, &msetnx},
    {"setnx", kPAttrWrite, 3, &setnx},
    {"setex", kPAttrWrite, 4, &setex},
    {"psetex", kPAttrWrite, 4, &psetex},
    {"getset", kPAttrWrite, 3, &getset},
    {"mget", kPAttrRead, -2, &mget},
    {"append", kPAttrWrite, 3, &append},
    {"bitcount", kPAttrRead, -2, &bitcount},
//        {"bitop", PAttr_write, -4, &bitop},
    {"getbit", kPAttrRead, 3, &getbit},
    {"setbit", kPAttrWrite, 4, &setbit},
    {"incr", kPAttrWrite, 2, &incr},
    {"decr", kPAttrWrite, 2, &decr},
    {"incrby", kPAttrWrite, 3, &incrby},
    {"incrbyfloat", kPAttrWrite, 3, &incrbyfloat},
    {"decrby", kPAttrWrite, 3, &decrby},
    {"getrange", kPAttrRead, 4, &getrange},
    {"setrange", kPAttrWrite, 4, &setrange},

    // list
    {"lpush", kPAttrWrite, -3, &lpush},
    {"rpush", kPAttrWrite, -3, &rpush},
    {"lpushx", kPAttrWrite, -3, &lpushx},
    {"rpushx", kPAttrWrite, -3, &rpushx},
    {"lpop", kPAttrWrite, 2, &lpop},
    {"rpop", kPAttrWrite, 2, &rpop},
    {"lindex", kPAttrRead, 3, &lindex},
    {"llen", kPAttrRead, 2, &llen},
    {"lset", kPAttrWrite, 4, &lset},
    {"ltrim", kPAttrWrite, 4, &ltrim},
    {"lrange", kPAttrRead, 4, &lrange},
    {"linsert", kPAttrWrite, 5, &linsert},
    {"lrem", kPAttrWrite, 4, &lrem},
    {"rpoplpush", kPAttrWrite, 3, &rpoplpush},
    {"blpop", kPAttrWrite, -3, &blpop},
    {"brpop", kPAttrWrite, -3, &brpop},
    {"brpoplpush", kPAttrWrite, 4, &brpoplpush},

    // hash
    {"hget", kPAttrRead, 3, &hget},
    {"hgetall", kPAttrRead, 2, &hgetall},
    {"hmget", kPAttrRead, -3, &hmget},
    {"hset", kPAttrWrite, 4, &hset},
    {"hsetnx", kPAttrWrite, 4, &hsetnx},
    {"hmset", kPAttrWrite, -4, &hmset},
    {"hlen", kPAttrRead, 2, &hlen},
    {"hexists", kPAttrRead, 3, &hexists},
    {"hkeys", kPAttrRead, 2, &hkeys},
    {"hvals", kPAttrRead, 2, &hvals},
    {"hdel", kPAttrWrite, -3, &hdel},
    {"hincrby", kPAttrWrite, 4, &hincrby},
    {"hincrbyfloat", kPAttrWrite, 4, &hincrbyfloat},
    {"hscan", kPAttrRead, -3, &hscan},
    {"hstrlen", kPAttrRead, 3, &hstrlen},

    // set
    {"sadd", kPAttrWrite, -3, &sadd},
    {"scard", kPAttrRead, 2, &scard},
    {"sismember", kPAttrRead, 3, &sismember},
    {"srem", kPAttrWrite, -3, &srem},
    {"smembers", kPAttrRead, 2, &smembers},
    {"sdiff", kPAttrRead, -2, &sdiff},
    {"sdiffstore", kPAttrWrite, -3, &sdiffstore},
    {"sinter", kPAttrRead, -2, &sinter},
    {"sinterstore", kPAttrWrite, -3, &sinterstore},
    {"sunion", kPAttrRead, -2, &sunion},
    {"sunionstore", kPAttrWrite, -3, &sunionstore},
    {"smove", kPAttrWrite, 4, &smove},
    {"spop", kPAttrWrite, 2, &spop},
    {"srandmember", kPAttrRead, 2, &srandmember},
    {"sscan", kPAttrRead, -3, &sscan},

    // zset
    {"zadd", kPAttrWrite, -4, &zadd},
    {"zcard", kPAttrRead, 2, &zcard},
    {"zrank", kPAttrRead, 3, &zrank},
    {"zrevrank", kPAttrRead, 3, &zrevrank},
    {"zrem", kPAttrWrite, -3, &zrem},
    {"zincrby", kPAttrWrite, 4, &zincrby},
    {"zscore", kPAttrRead, 3, &zscore},
    {"zrange", kPAttrRead, -4, &zrange},
    {"zrevrange", kPAttrRead, -4, &zrevrange},
    {"zrangebyscore", kPAttrRead, -4, &zrangebyscore},
    {"zrevrangebyscore", kPAttrRead, -4, &zrevrangebyscore},
    {"zremrangebyrank", kPAttrWrite, 4, &zremrangebyrank},
    {"zremrangebyscore", kPAttrWrite, 4, &zremrangebyscore},

    // pubsub
    {"subscribe", kPAttrRead, -2, &subscribe},
    {"unsubscribe", kPAttrRead, -1, &unsubscribe},
    {"publish", kPAttrRead, 3, &publish},
    {"psubscribe", kPAttrRead, -2, &psubscribe},
    {"punsubscribe", kPAttrRead, -1, &punsubscribe},
    {"pubsub", kPAttrRead, -2, &pubsub},

    // multi
    {"watch", kPAttrRead, -2, &watch},
    {"unwatch", kPAttrRead, 1, &unwatch},
    {"multi", kPAttrRead, 1, &multi},
    {"exec", kPAttrRead, 1, &exec},
    {"discard", kPAttrRead, 1, &discard},

    // replication
    {"sync", kPAttrRead, 1, &sync},
    {"psync", kPAttrRead, 1, &sync},
    {"slaveof", kPAttrRead, 3, &slaveof},
    {"replconf", kPAttrRead, -3, &replconf},

    // help
    {"cmdlist", kPAttrRead, 1, &cmdlist},
};

Delegate<void(UnboundedBuffer&)> g_infoCollector;

std::map<PString, const PCommandInfo*, NocaseComp> PCommandTable::s_handlers;

PCommandTable::PCommandTable() { Init(); }

void PCommandTable::Init() {
  for (const auto& info : s_info) {
    s_handlers[info.cmd] = &info;
  }

  g_infoCollector += OnMemoryInfoCollect;
  g_infoCollector += OnServerInfoCollect;
  g_infoCollector += OnClientInfoCollect;
  g_infoCollector += std::bind(&PReplication::OnInfoCommand, &PREPL, std::placeholders::_1);
}

const PCommandInfo* PCommandTable::GetCommandInfo(const PString& cmd) {
  auto it(s_handlers.find(cmd));
  if (it != s_handlers.end()) {
    return it->second;
  }

  return nullptr;
}

bool PCommandTable::AliasCommand(const std::map<PString, PString>& aliases) {
  for (const auto& pair : aliases) {
    if (!AliasCommand(pair.first, pair.second)) {
      return false;
    }
  }

  return true;
}

bool PCommandTable::AliasCommand(const PString& oldKey, const PString& newKey) {
  auto info = DelCommand(oldKey);
  if (!info) {
    return false;
  }

  return AddCommand(newKey, info);
}

const PCommandInfo* PCommandTable::DelCommand(const PString& cmd) {
  auto it(s_handlers.find(cmd));
  if (it != s_handlers.end()) {
    auto p = it->second;
    s_handlers.erase(it);
    return p;
  }

  return nullptr;
}

bool PCommandTable::AddCommand(const PString& cmd, const PCommandInfo* info) {
  if (cmd.empty() || cmd == "\"\"") {
    return true;
  }

  return s_handlers.insert(std::make_pair(cmd, info)).second;
}

PError PCommandTable::ExecuteCmd(const std::vector<PString>& params, const PCommandInfo* info, UnboundedBuffer* reply) {
  if (params.empty()) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  if (!info) {
    ReplyError(kPErrorUnknowCmd, reply);
    return kPErrorUnknowCmd;
  }

  if (!info->CheckParamsCount(static_cast<int>(params.size()))) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  return info->handler(params, reply);
}

PError PCommandTable::ExecuteCmd(const std::vector<PString>& params, UnboundedBuffer* reply) {
  if (params.empty()) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  auto it(s_handlers.find(params[0]));
  if (it == s_handlers.end()) {
    ReplyError(kPErrorUnknowCmd, reply);
    return kPErrorUnknowCmd;
  }

  const PCommandInfo* info = it->second;
  if (!info->CheckParamsCount(static_cast<int>(params.size()))) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  return info->handler(params, reply);
}

bool PCommandInfo::CheckParamsCount(int nParams) const {
  if (params > 0) {
    return params == nParams;
  }

  return nParams + params >= 0;
}

PError cmdlist(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PreFormatMultiBulk(PCommandTable::s_handlers.size(), reply);
  for (const auto& kv : PCommandTable::s_handlers) {
    FormatBulk(kv.first, reply);
  }

  return kPErrorOK;
}

}  // namespace pikiwidb
