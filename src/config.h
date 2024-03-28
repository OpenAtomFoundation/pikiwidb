/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common.h"

namespace pikiwidb {

enum BackEndType {
  kBackEndNone = 0,
  kBackEndRocksDB = 1,
  kBackEndMax = 2,
};

struct PConfig {
  bool daemonize;
  PString pidfile;

  PString ip;
  unsigned short port;

  int timeout;

  PString dbpath;

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

  int fast_cmd_threads_num;
  int slow_cmd_threads_num;

  int backend;  // enum BackEndType
  PString backendPath;
  int backendHz;  // the frequency of dump to backend

  int64_t max_client_response_size;

  int db_instance_num;
  uint64_t rocksdb_ttl_second;
  uint64_t rocksdb_periodic_second;
  PConfig();

  bool CheckArgs() const;
  bool CheckPassword(const PString& pwd) const;
};

extern PConfig g_config;

extern bool LoadPikiwiDBConfig(const char* cfgFile, PConfig& cfg);

}  // namespace pikiwidb
