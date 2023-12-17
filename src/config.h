/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <map>
#include <vector>
#include "pstring.h"

namespace pikiwidb {

enum BackEndType {
  kBackEndNone = 0,
  kBackEndRocksdb = 1,
  kBackEndMax = 2,
};

struct PConfig {
  bool daemonize;
  PString pidfile;

  PString ip;
  unsigned short port;

  int timeout;

  PString loglevel;
  PString logdir;  // the log directory, differ from redis

  int databases;

  // auth
  PString password;

  std::map<PString, PString> aliases;

  // @ rdb
  // save seconds changes
  int saveseconds;
  int savechanges;
  bool rdbcompression;  // yes
  bool rdbchecksum;     // yes
  PString rdbfullname;  // ./dump.rdb

  int maxclients;  // 10000

  int slowlogtime;    // 1000 microseconds
  int slowlogmaxlen;  // 128

  int hz;  // 10  [1,500]

  PString masterIp;
  unsigned short masterPort;  // replication
  PString masterauth;

  PString runid;

  PString includefile;  // the template config

  std::vector<PString> modules;  // modules

  // use redis as cache, level db as backup
  uint64_t maxmemory;    // default 2GB
  int maxmemorySamples;  // default 5
  bool noeviction;       // default true

  // THREADED I/O
  int worker_threads_num;

  // THREADED SLAVE
  int slave_threads_num;

  int backend;  // enum BackEndType
  PString backendPath;
  int backendHz;  // the frequency of dump to backend

  // cache
  int cache_num;
  int cache_model;
  bool tmp_cache_disable_flag{false};
  int cache_string;
  int cache_set;
  int cache_zset;
  int cache_hash;
  int cache_list;
  int cache_bit;
  int zset_cache_field_num_per_key;
  int zset_cache_start_pos;
  int64_t cache_maxmemory;
  int cache_maxmemory_policy;
  int cache_maxmemory_samples;
  int cache_lfu_decay_time;

  PConfig();

  bool CheckArgs() const;
  bool CheckPassword(const PString& pwd) const;
  bool IsCacheDisabledTemporarily() const { return tmp_cache_disable_flag; }
  void UnsetCacheDisableFlag() { tmp_cache_disable_flag = false; }
  void SetCacheDisableFlag() { tmp_cache_disable_flag = true; }
  int GetCacheString() const { return cache_string; }
  int GetCacheSet() const { return cache_set; }
  int GetCacheZset() const { return cache_zset; }
  int GetCacheHash() const { return cache_hash; }
  int GetCacheList() const { return cache_list; }
  int GetCacheBit() const { return cache_bit; }
  int GetCacheNum() const { return cache_num; }
  int GetCacheModel() const { return cache_model; }
};

extern PConfig g_config;

extern bool LoadPikiwiDBConfig(const char* cfgFile, PConfig& cfg);

}  // namespace pikiwidb
