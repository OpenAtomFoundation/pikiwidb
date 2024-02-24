/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//
//  PikiwiDB.cc

#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <thread>

#include "log.h"
#include "rocksdb/db.h"

#include "client.h"
#include "command.h"
#include "store.h"

#include "config.h"
#include "db.h"
#include "pubsub.h"
#include "slow_log.h"

#include "pikiwidb.h"
#include "pikiwidb_logo.h"
#include "pstd_util.h"

std::unique_ptr<PikiwiDB> g_pikiwidb;

static void IntSigHandle(const int sig) {
  INFO("Catch Signal {}, cleanup...", sig);
  g_pikiwidb->Stop();
}

static void SignalSetup() {
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, &IntSigHandle);
  signal(SIGQUIT, &IntSigHandle);
  signal(SIGTERM, &IntSigHandle);
}

const uint32_t PikiwiDB::kRunidSize = 40;

static void Usage() {
  std::cerr << "Usage:  ./pikiwidb-server [/path/to/redis.conf] [options]\n\
        ./pikiwidb-server -v or --version\n\
        ./pikiwidb-server -h or --help\n\
Examples:\n\
        ./pikiwidb-server (run the server with default conf)\n\
        ./pikiwidb-server /etc/redis/6379.conf\n\
        ./pikiwidb-server --port 7777\n\
        ./pikiwidb-server --port 7777 --slaveof 127.0.0.1 8888\n\
        ./pikiwidb-server /etc/myredis.conf --loglevel verbose\n";
}

bool PikiwiDB::ParseArgs(int ac, char* av[]) {
  for (int i = 0; i < ac; i++) {
    if (cfg_file_.empty() && ::access(av[i], R_OK) == 0) {
      cfg_file_ = av[i];
      continue;
    } else if (strncasecmp(av[i], "-v", 2) == 0 || strncasecmp(av[i], "--version", 9) == 0) {
      std::cerr << "PikiwiDB Server v=" << kPIKIWIDB_VERSION << " bits=" << (sizeof(void*) == 8 ? 64 : 32) << std::endl;

      exit(0);
      return true;
    } else if (strncasecmp(av[i], "-h", 2) == 0 || strncasecmp(av[i], "--help", 6) == 0) {
      Usage();
      exit(0);
      return true;
    } else if (strncasecmp(av[i], "--port", 6) == 0) {
      if (++i == ac) {
        return false;
      }
      port_ = static_cast<int16_t>(std::atoi(av[i]));
    } else if (strncasecmp(av[i], "--loglevel", 10) == 0) {
      if (++i == ac) {
        return false;
      }
      log_level_ = std::string(av[i]);
    } else if (strncasecmp(av[i], "--slaveof", 9) == 0) {
      if (i + 2 >= ac) {
        return false;
      }

      master_ = std::string(av[++i]);
      master_port_ = static_cast<int16_t>(std::atoi(av[++i]));
    } else {
      std::cerr << "Unknow option " << av[i] << std::endl;
      return false;
    }
  }

  return true;
}

static void PdbCron() {
  using namespace pikiwidb;

  if (g_qdbPid != -1) {
    return;
  }

  if (Now() > (g_lastPDBSave + static_cast<unsigned>(g_config.saveseconds)) * 1000UL &&
      PStore::dirty_ >= g_config.savechanges) {
    int ret = fork();
    if (ret == 0) {
      {
        PDBSaver qdb;
        qdb.Save(g_config.rdbfullname.c_str());
        std::cerr << "ServerCron child save rdb done, exiting child\n";
      }  //  make qdb to be destructed before exit
      _exit(0);
    } else if (ret == -1) {
      ERROR("fork qdb save process failed");
    } else {
      g_qdbPid = ret;
    }

    INFO("ServerCron save rdb file {}", g_config.rdbfullname);
  }
}

static void LoadDBFromDisk() {
  using namespace pikiwidb;

  PDBLoader loader;
  loader.Load(g_config.rdbfullname.c_str());
}

static void CheckChild() {
  using namespace pikiwidb;

  if (g_qdbPid == -1) {
    return;
  }

  int statloc = 0;
  pid_t pid = wait3(&statloc, WNOHANG, nullptr);

  if (pid != 0 && pid != -1) {
    int exit = WEXITSTATUS(statloc);
    int signal = 0;

    if (WIFSIGNALED(statloc)) {
      signal = WTERMSIG(statloc);
    }

    if (pid == g_qdbPid) {
      PDBSaver::SaveDoneHandler(exit, signal);
      if (PREPL.IsBgsaving()) {
        PREPL.OnRdbSaveDone();
      } else {
        PREPL.TryBgsave();
      }
    } else {
      ERROR("{} is not rdb process", pid);
      assert(!!!"Is there any back process except rdb?");
    }
  }
}

void PikiwiDB::OnNewConnection(pikiwidb::TcpConnection* obj) {
  INFO("New connection from {}:{}", obj->GetPeerIp(), obj->GetPeerPort());

  auto client = std::make_shared<pikiwidb::PClient>(obj);
  obj->SetContext(client);

  client->OnConnect();

  auto msg_cb = std::bind(&pikiwidb::PClient::HandlePackets, client.get(), std::placeholders::_1, std::placeholders::_2,
                          std::placeholders::_3);
  obj->SetMessageCallback(msg_cb);
  obj->SetOnDisconnect([](pikiwidb::TcpConnection* obj) { INFO("disconnect from {}", obj->GetPeerIp()); });
  obj->SetNodelay(true);
  obj->SetEventLoopSelector([this]() { return worker_threads_.ChooseNextWorkerEventLoop(); });
  obj->SetSlaveEventLoopSelector([this]() { return slave_threads_.ChooseNextWorkerEventLoop(); });
}

bool PikiwiDB::Init() {
  using namespace pikiwidb;

  char runid[kRunidSize + 1] = "";
  getRandomHexChars(runid, kRunidSize);
  g_config.runid.assign(runid, kRunidSize);

  if (port_ != 0) {
    g_config.port = port_;
  }

  if (!log_level_.empty()) {
    g_config.loglevel = log_level_;
  }

  if (!master_.empty()) {
    g_config.masterIp = master_;
    g_config.masterPort = master_port_;
  }

  NewTcpConnectionCallback cb = std::bind(&PikiwiDB::OnNewConnection, this, std::placeholders::_1);
  if (!worker_threads_.Init(g_config.ip.c_str(), g_config.port, cb)) {
    return false;
  }

  auto num = g_config.worker_threads_num + g_config.slave_threads_num;
  auto kMaxWorkerNum = IOThreadPool::GetMaxWorkerNum();
  if (num > kMaxWorkerNum) {
    ERROR("number of threads can't exceeds {}, now is {}", kMaxWorkerNum, num);
    return false;
  }
  worker_threads_.SetWorkerNum(static_cast<size_t>(g_config.worker_threads_num));
  slave_threads_.SetWorkerNum(static_cast<size_t>(g_config.slave_threads_num));

  PCommandTable::Init();
  PCommandTable::AliasCommand(g_config.aliases);
  PSTORE.Init(g_config.databases);
  PSTORE.InitExpireTimer();
  PSTORE.InitBlockedTimer();
  PSTORE.InitEvictionTimer();
  PSTORE.InitDumpBackends();
  PPubsub::Instance().InitPubsubTimer();

  // Only if there is no backend, load rdb
  if (g_config.backend == pikiwidb::kBackEndNone) {
    LoadDBFromDisk();
  }

  PSlowLog::Instance().SetThreshold(g_config.slowlogtime);
  PSlowLog::Instance().SetLogLimit(static_cast<std::size_t>(g_config.slowlogmaxlen));

  // init base loop
  auto loop = worker_threads_.BaseLoop();
  loop->ScheduleRepeatedly(1000 / pikiwidb::g_config.hz, PdbCron);
  loop->ScheduleRepeatedly(1000, &PReplication::Cron, &PREPL);
  loop->ScheduleRepeatedly(1, CheckChild);

  // master ip
  if (!g_config.masterIp.empty()) {
    PREPL.SetMasterAddr(g_config.masterIp.c_str(), g_config.masterPort);
  }

  cmd_table_manager_.InitCmdTable();

  return true;
}

void PikiwiDB::Run() {
  worker_threads_.SetName("pikiwi-main");
  slave_threads_.SetName("pikiwi-slave");

  std::thread t([this]() {
    auto slave_loop = slave_threads_.BaseLoop();
    slave_loop->Init();
    slave_threads_.Run(0, nullptr);
  });

  worker_threads_.Run(0, nullptr);

  t.join();  // wait for slave thread exit
  INFO("server exit running");
}

void PikiwiDB::Stop() {
  pikiwidb::PRAFT.ShutDown();
  pikiwidb::PRAFT.Join();
  slave_threads_.Exit();
  worker_threads_.Exit();
}

pikiwidb::CmdTableManager& PikiwiDB::GetCmdTableManager() { return cmd_table_manager_; }

static void InitLogs() {
  logger::Init("logs/pikiwidb_server.log");

#if BUILD_DEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

static void daemonize() {
  if (fork()) {
    exit(0); /* parent exits */
  }
  setsid(); /* create a new session */
}

static void closeStd() {
  int fd;
  fd = open("/dev/null", O_RDWR, 0);
  if (fd != -1) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
  }
}

int main(int ac, char* av[]) {
  [[maybe_unused]] rocksdb::DB* db;
  g_pikiwidb = std::make_unique<PikiwiDB>();
  if (!g_pikiwidb->ParseArgs(ac - 1, av + 1)) {
    Usage();
    return -1;
  }

  if (!g_pikiwidb->GetConfigName().empty()) {
    if (!LoadPikiwiDBConfig(g_pikiwidb->GetConfigName().c_str(), pikiwidb::g_config)) {
      std::cerr << "Load config file [" << g_pikiwidb->GetConfigName() << "] failed!\n";
      return -1;
    }
  }

  // output logo to console
  char logo[512] = "";
  snprintf(logo, sizeof logo - 1, pikiwidbLogo, kPIKIWIDB_VERSION, static_cast<int>(sizeof(void*)) * 8,
           static_cast<int>(pikiwidb::g_config.port));
  std::cout << logo;

  if (pikiwidb::g_config.daemonize) {
    daemonize();
  }

  pstd::InitRandom();
  SignalSetup();
  InitLogs();

  if (pikiwidb::g_config.daemonize) {
    closeStd();
  }

  if (g_pikiwidb->Init()) {
    g_pikiwidb->Run();
  }

  // when process exit, flush log
  spdlog::get(logger::Logger::Instance().Name())->flush();
  return 0;
}
