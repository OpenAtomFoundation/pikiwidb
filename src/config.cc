/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>

#include "config.h"
#include "common.h"
#include "config_parser.h"
#include "pstd_string.h"

namespace pikiwidb {

static void EraseQuotes(PString& str) {
  // convert "hello" to  hello
  if (str.size() < 2) {
    return;
  }
  if (str[0] == '"' && str[str.size() - 1] == '"') {
    str.erase(str.begin());
    str.pop_back();
  }
}

extern std::vector<PString> SplitString(const PString& str, char seperator);

PConfig g_config;

PConfig::PConfig() {
  daemonize = false;
  pidfile = "/var/run/pikiwidb.pid";

  ip = "127.0.0.1";
  port = 9221;
  timeout = 0;
  dbpath = "./db";

  loglevel = "notice";
  logdir = "stdout";

  databases = 16;

  // rdb
  saveseconds = 999999999;
  savechanges = 999999999;
  rdbcompression = true;
  rdbchecksum = true;
  rdbfullname = "./dump.rdb";

  maxclients = 10000;

  // slow log
  slowlogtime = 0;
  slowlogmaxlen = 128;

  hz = 10;

  includefile = "";

  maxmemory = 2 * 1024 * 1024 * 1024UL;
  maxmemorySamples = 5;
  noeviction = true;

  backend = kBackEndRocksdb;
  backendPath = "dump";
  backendHz = 10;

  max_client_response_size = 1073741824;
}

bool LoadPikiwiDBConfig(const char* cfgFile, PConfig& cfg) {
  ConfigParser parser;
  if (!parser.Load(cfgFile)) {
    return false;
  }

  if (parser.GetData<PString>("daemonize") == "yes") {
    cfg.daemonize = true;
  } else {
    cfg.daemonize = false;
  }

  cfg.pidfile = parser.GetData<PString>("pidfile", cfg.pidfile);

  cfg.ip = parser.GetData<PString>("bind", cfg.ip);
  cfg.port = parser.GetData<unsigned short>("port");
  cfg.timeout = parser.GetData<int>("timeout");
  cfg.dbpath = parser.GetData<PString>("db-path");

  cfg.loglevel = parser.GetData<PString>("loglevel", cfg.loglevel);
  cfg.logdir = parser.GetData<PString>("logfile", cfg.logdir);
  EraseQuotes(cfg.logdir);
  if (cfg.logdir.empty()) {
    cfg.logdir = "stdout";
  }

  cfg.databases = parser.GetData<int>("databases", cfg.databases);
  cfg.password = parser.GetData<PString>("requirepass");
  EraseQuotes(cfg.password);

  // alias command
  {
    std::vector<PString> alias(SplitString(parser.GetData<PString>("rename-command"), ' '));
    if (alias.size() % 2 == 0) {
      for (auto it(alias.begin()); it != alias.end();) {
        const PString& oldCmd = *(it++);
        const PString& newCmd = *(it++);
        cfg.aliases[oldCmd] = newCmd;
      }
    }
  }

  // load rdb config
  std::vector<PString> saveInfo(SplitString(parser.GetData<PString>("save"), ' '));
  if (!saveInfo.empty() && saveInfo.size() != 2) {
    EraseQuotes(saveInfo[0]);
    if (!(saveInfo.size() == 1 && saveInfo[0].empty())) {
      std::cerr << "bad format save rdb interval, bad string " << parser.GetData<PString>("save") << std::endl;
      return false;
    }
  } else if (!saveInfo.empty()) {
    cfg.saveseconds = std::stoi(saveInfo[0]);
    cfg.savechanges = std::stoi(saveInfo[1]);
  }

  if (cfg.saveseconds == 0) {
    cfg.saveseconds = 999999999;
  }
  if (cfg.savechanges == 0) {
    cfg.savechanges = 999999999;
  }

  cfg.rdbcompression = (parser.GetData<PString>("rdbcompression") == "yes");
  cfg.rdbchecksum = (parser.GetData<PString>("rdbchecksum") == "yes");

  cfg.rdbfullname = parser.GetData<PString>("dir", "./") + parser.GetData<PString>("dbfilename", "dump.rdb");

  cfg.maxclients = parser.GetData<int>("maxclients", 10000);

  cfg.slowlogtime = parser.GetData<int>("slowlog-log-slower-than", 0);
  cfg.slowlogmaxlen = parser.GetData<int>("slowlog-max-len", cfg.slowlogmaxlen);

  cfg.hz = parser.GetData<int>("hz", 10);

  cfg.max_client_response_size = parser.GetData<int64_t>("max-client-response-size");

  // load master ip port
  std::vector<PString> master(SplitString(parser.GetData<PString>("slaveof"), ' '));
  if (master.size() == 2) {
    cfg.masterIp = std::move(master[0]);
    cfg.masterPort = static_cast<unsigned short>(std::stoi(master[1]));
  }
  cfg.masterauth = parser.GetData<PString>("masterauth");

  // load modules' names
  cfg.modules = parser.GetDataVector("loadmodule");

  cfg.includefile = parser.GetData<PString>("include");  // TODO multi files include

  // lru cache
  cfg.maxmemory = parser.GetData<uint64_t>("maxmemory", 2 * 1024 * 1024 * 1024UL);
  cfg.maxmemorySamples = parser.GetData<int>("maxmemory-samples", 5);
  cfg.noeviction = (parser.GetData<PString>("maxmemory-policy", "noeviction") == "noeviction");

  // worker threads
  cfg.worker_threads_num = parser.GetData<int>("worker-threads", 1);

  // slave threads
  cfg.slave_threads_num = parser.GetData<int>("slave-threads", 1);

  // backend
  cfg.backend = parser.GetData<int>("backend", kBackEndNone);
  cfg.backendPath = parser.GetData<PString>("backendpath", cfg.backendPath);
  EraseQuotes(cfg.backendPath);
  cfg.backendHz = parser.GetData<int>("backendhz", 10);

  // cache-num
  cfg.cache_num = parser.GetData<int>("cache-num", 16);
  if (cfg.cache_num <= 0 || cfg.cache_num > 48) {
    cfg.cache_num = 16;
  }

  // cache-model
  cfg.cache_model = parser.GetData<int>("cache-model", 0);

  // cache-type
  auto cacheTypeValue = parser.GetData<PString>("cache-type");
  pstd::StringToLower(cacheTypeValue);
  std::set<std::string> available_types = {"string", "set", "zset", "list", "hash", "bit"};
  std::string type_str = cacheTypeValue;
  std::vector<std::string> types;
  type_str.erase(remove_if(type_str.begin(), type_str.end(), isspace), type_str.end());
  pstd::StringSplit(type_str, COMMA, types);
  for (auto& type : types) {
    if (available_types.find(type) == available_types.end()) {
      std::cerr << "-ERR Invalid cache type: " << type << std::endl;
      return false;
    }

    if (type == "string") {
      cfg.cache_string = 1;
    } else if (type == "set") {
      cfg.cache_set = 1;
    } else if (type == "zset") {
      cfg.cache_zset = 1;
    } else if (type == "hash") {
      cfg.cache_hash = 1;
    } else if (type == "list") {
      cfg.cache_list = 1;
    } else if (type == "bit") {
      cfg.cache_bit = 1;
    }
  }

  // zset-cache-field-num-per-key
  cfg.zset_cache_field_num_per_key = parser.GetData<int>("zset-cache-field-num-per-key", 512);

  // zset-cache-start-direction
  cfg.zset_cache_start_pos = parser.GetData<int>("zset-cache-start-direction", 0);

  // cache-maxmemory
  cfg.cache_maxmemory = parser.GetData<int64_t>("cache-maxmemory", PIKIWIDB_CACHE_SIZE_DEFAULT);

  // cache-maxmemory-policy
  cfg.cache_maxmemory_policy = parser.GetData<int>("cache-maxmemory-policy", 1);

  // cache-maxmemory-samples
  cfg.cache_maxmemory_samples = parser.GetData<int>("cache-maxmemory-samples", 5);

  // cache-lfu-decay-time
  cfg.cache_lfu_decay_time = parser.GetData<int>("cache-lfu-decay-time", 1);

  return cfg.CheckArgs();
}

bool PConfig::CheckArgs() const {
#define RETURN_IF_FAIL(cond)        \
  if (!(cond)) {                    \
    std::cerr << #cond " failed\n"; \
    return false;                   \
  }

  RETURN_IF_FAIL(port > 0);
  RETURN_IF_FAIL(databases > 0);
  RETURN_IF_FAIL(maxclients > 0);
  RETURN_IF_FAIL(hz > 0 && hz < 500);
  RETURN_IF_FAIL(maxmemory >= 512 * 1024 * 1024UL);
  RETURN_IF_FAIL(maxmemorySamples > 0 && maxmemorySamples < 10);
  RETURN_IF_FAIL(worker_threads_num > 0 && slave_threads_num > 0 && (worker_threads_num + slave_threads_num) < 129);  // as redis
  RETURN_IF_FAIL(backend >= kBackEndNone && backend < kBackEndMax);
  RETURN_IF_FAIL(backendHz >= 1 && backendHz <= 50);
  RETURN_IF_FAIL(cache_num > 0 && cache_num <= 48);
  RETURN_IF_FAIL(cache_model == PIKIWIDB_CACHE_NONE || cache_model == PIKIWIDB_CACHE_READ);
  RETURN_IF_FAIL(cache_string == 0 || cache_string == 1);
  RETURN_IF_FAIL(cache_set == 0 || cache_set == 1);
  RETURN_IF_FAIL(cache_zset == 0 || cache_zset == 1);
  RETURN_IF_FAIL(cache_hash == 0 || cache_hash == 1);
  RETURN_IF_FAIL(cache_list == 0 || cache_list == 1);
  RETURN_IF_FAIL(cache_bit == 0 || cache_bit == 1);
  RETURN_IF_FAIL(zset_cache_field_num_per_key >= 0);
  RETURN_IF_FAIL(zset_cache_start_pos == CACHE_START_FROM_BEGIN || zset_cache_start_pos == CACHE_START_FROM_END);
  RETURN_IF_FAIL(cache_maxmemory >= PIKIWIDB_CACHE_SIZE_MIN);
  RETURN_IF_FAIL(cache_maxmemory_policy >= 0 || cache_maxmemory_policy <= 5);
  RETURN_IF_FAIL(cache_maxmemory_samples > 0);
  RETURN_IF_FAIL(cache_lfu_decay_time > 0);

#undef RETURN_IF_FAIL

  return true;
}

bool PConfig::CheckPassword(const PString& pwd) const { return password.empty() || password == pwd; }

}  // namespace pikiwidb
