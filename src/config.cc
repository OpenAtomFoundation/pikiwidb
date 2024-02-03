/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <iostream>
#include <vector>

#include "config.h"
#include "config_parser.h"

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

  backend = kBackEndRocksDB;
  backendPath = "dump";
  backendHz = 10;

  max_client_response_size = 1073741824;

  db_instance_num = 3;

  rocksdb_ttl_second = 0;
  rocksdb_periodic_second = 0;
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
  cfg.fast_cmd_threads_num = parser.GetData<int>("fast-threads", 1);
  cfg.slow_cmd_threads_num = parser.GetData<int>("slow-threads", 1);

  // backend
  cfg.backend = parser.GetData<int>("backend", kBackEndNone);
  cfg.backendPath = parser.GetData<PString>("backendpath", cfg.backendPath);
  EraseQuotes(cfg.backendPath);
  cfg.backendHz = parser.GetData<int>("backendhz", 10);

  cfg.max_client_response_size = parser.GetData<int64_t>("max-client-response-size", 1073741824);

  cfg.db_instance_num = parser.GetData<int>("db-instance-num", 3);
  cfg.rocksdb_ttl_second = parser.GetData<uint64_t>("rocksdb-ttl-second");
  cfg.rocksdb_periodic_second = parser.GetData<uint64_t>("rocksdb-periodic-second");

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
  RETURN_IF_FAIL(worker_threads_num > 0 && worker_threads_num < 129);  // as redis
  RETURN_IF_FAIL(fast_cmd_threads_num > 0 && fast_cmd_threads_num < 16);
  RETURN_IF_FAIL(slow_cmd_threads_num > 0 && slow_cmd_threads_num < 16);
  RETURN_IF_FAIL(backend >= kBackEndNone && backend < kBackEndMax);
  RETURN_IF_FAIL(backendHz >= 1 && backendHz <= 50);
  RETURN_IF_FAIL(db_instance_num >= 1);
  RETURN_IF_FAIL(rocksdb_ttl_second > 0);
  RETURN_IF_FAIL(rocksdb_periodic_second > 0);
  RETURN_IF_FAIL(max_client_response_size > 0);

#undef RETURN_IF_FAIL

  return true;
}

bool PConfig::CheckPassword(const PString& pwd) const { return password.empty() || password == pwd; }

}  // namespace pikiwidb
