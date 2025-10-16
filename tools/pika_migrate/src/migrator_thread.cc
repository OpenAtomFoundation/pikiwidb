// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "include/migrator_thread.h"

#include <unistd.h>

#include <vector>
#include <functional>
#define GLOG_USE_GLOG_EXPORT
#include <glog/logging.h>

#include "storage/storage.h"
#include "storage/src/redis.h"
#include "src/scope_snapshot.h"
#include "src/strings_value_format.h"

#include "include/pika_conf.h"

const int64_t MAX_BATCH_NUM = 30000;

extern PikaConf* g_pika_conf;

MigratorThread::~MigratorThread() {
}

void MigratorThread::MigrateStringsDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::string value;
  std::vector<std::string> keys;
  int64_t timestamp;
  while (true) {
    cursor = storage_->Scan(storage::DataType::kStrings, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      s = storage_->Get(key, &value);
      if (!s.ok()) {
        LOG(WARNING) << "get " << key << " error: " << s.ToString();
        continue;
      }

      net::RedisCmdArgsType argv;
      std::string cmd;

      argv.push_back("SET");
      argv.push_back(key);
      argv.push_back(value);

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

      if (ttl > 0) {
        argv.push_back("EX");
        argv.push_back(std::to_string(ttl));
      }

      net::SerializeRedisCommand(argv, &cmd);
      PlusNum();
      DispatchKey(cmd, key);
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateListsDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::vector<std::string> keys;
  int64_t timestamp;

  while (true) {
    cursor = storage_->Scan(storage::DataType::kLists, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      int64_t pos = 0;
      std::vector<std::string> nodes;
      storage::Status s = storage_->LRange(key, pos, pos + g_pika_conf->sync_batch_num() - 1, &nodes);
      if (!s.ok()) {
        LOG(WARNING) << "db->LRange(key:" << key << ", pos:" << pos
          << ", batch size: " << g_pika_conf->sync_batch_num() << ") = " << s.ToString();
        continue;
      }

      while (s.ok() && !should_exit_ && !nodes.empty()) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("RPUSH");
        argv.push_back(key);
        for (const auto& node : nodes) {
          argv.push_back(node);
        }

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);

        pos += g_pika_conf->sync_batch_num();
        nodes.clear();
        s = storage_->LRange(key, pos, pos + g_pika_conf->sync_batch_num() - 1, &nodes);
        if (!s.ok()) {
          LOG(WARNING) << "db->LRange(key:" << key << ", pos:" << pos
            << ", batch size:" << g_pika_conf->sync_batch_num() << ") = " << s.ToString();
        }
      }

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

      if (s.ok() && ttl > 0) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("EXPIRE");
        argv.push_back(key);
        argv.push_back(std::to_string(ttl));

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateHashesDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::vector<std::string> keys;
  int64_t timestamp;

  while (true) {
    cursor = storage_->Scan(storage::DataType::kHashes, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      std::vector<storage::FieldValue> fvs;
      storage::Status s = storage_->HGetall(key, &fvs);
      if (!s.ok()) {
        LOG(WARNING) << "db->HGetall(key:" << key << ") = " << s.ToString();
        continue;
      }

      auto it = fvs.begin();
      while (!should_exit_ && it != fvs.end()) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("HMSET");
        argv.push_back(key);
        for (int idx = 0;
             idx < g_pika_conf->sync_batch_num() && !should_exit_ && it != fvs.end();
             idx++, it++) {
          argv.push_back(it->field);
          argv.push_back(it->value);
        }

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

      if (s.ok() && ttl > 0) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("EXPIRE");
        argv.push_back(key);
        argv.push_back(std::to_string(ttl));

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateSetsDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::vector<std::string> keys;
  int64_t timestamp;

  while (true) {
    cursor = storage_->Scan(storage::DataType::kSets, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      std::vector<std::string> members;
      storage::Status s = storage_->SMembers(key, &members);
      if (!s.ok()) {
        LOG(WARNING) << "db->SMembers(key:" << key << ") = " << s.ToString();
        continue;
      }
      auto it = members.begin();
      while (!should_exit_ && it != members.end()) {
        std::string cmd;
        net::RedisCmdArgsType argv;

        argv.push_back("SADD");
        argv.push_back(key);
        for (int idx = 0;
             idx < g_pika_conf->sync_batch_num() && !should_exit_ && it != members.end();
             idx++, it++) {
          argv.push_back(*it);
        }

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

      if (s.ok() && ttl > 0) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("EXPIRE");
        argv.push_back(key);
        argv.push_back(std::to_string(ttl));

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateZsetsDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::vector<std::string> keys;
  int64_t timestamp;

  while (true) {
    cursor = storage_->Scan(storage::DataType::kZSets, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      std::vector<storage::ScoreMember> score_members;
      storage::Status s = storage_->ZRange(key, 0, -1, &score_members);
      if (!s.ok()) {
        LOG(WARNING) << "db->ZRange(key:" << key << ") = " << s.ToString();
        continue;
      }
      auto it = score_members.begin();
      while (!should_exit_ && it != score_members.end()) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("ZADD");
        argv.push_back(key);
        for (int idx = 0;
             idx < g_pika_conf->sync_batch_num() && !should_exit_ && it != score_members.end();
             idx++, it++) {
          argv.push_back(std::to_string(it->score));
          argv.push_back(it->member);
        }

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

      if (s.ok() && ttl > 0) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("EXPIRE");
        argv.push_back(key);
        argv.push_back(std::to_string(ttl));

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateStreamsDB() {
  int64_t scan_batch_num = g_pika_conf->sync_batch_num() * 10;
  if (MAX_BATCH_NUM < scan_batch_num) {
    if (g_pika_conf->sync_batch_num() < MAX_BATCH_NUM) {
      scan_batch_num = MAX_BATCH_NUM;
    } else {
      scan_batch_num = g_pika_conf->sync_batch_num() * 2;
    }
  }

  int64_t ttl = -1;
  int64_t cursor = 0;
  storage::Status s;
  std::vector<std::string> keys;
  int64_t timestamp;

  while (true) {
    cursor = storage_->Scan(storage::DataType::kStreams, cursor, "*", scan_batch_num, &keys);

    for (const auto& key : keys) {
      std::vector<storage::IdMessage> id_message;
      storage::StreamScanArgs arg;
      storage::StreamUtils::StreamParseIntervalId("-", arg.start_sid, &arg.start_ex, 0);
      storage::StreamUtils::StreamParseIntervalId("+", arg.end_sid, &arg.end_ex, UINT64_MAX);
      
      storage::Status s = storage_->XRange(key, arg, id_message);
      if (!s.ok()) {
        LOG(WARNING) << "db->XRange(key:" << key << ") = " << s.ToString();
        continue;
      }
      auto it = id_message.begin();
      while (!should_exit_ && it != id_message.end()) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("XADD");
        argv.push_back(key);
        for (int idx = 0;
             idx < g_pika_conf->sync_batch_num() && !should_exit_ && it != id_message.end();
             idx++, it++) {
              std::vector<std::string> message;
              storage::StreamUtils::DeserializeMessage(it->value, message);
              storage::streamID sid;
              sid.DeserializeFrom(it->field);
              argv.push_back(sid.ToString());
              for (const auto& m : message) {
                argv.push_back(m);
              }
        }

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }

      ttl = -1;
      timestamp = storage_->TTL(key);
      if (timestamp != -2) {
        ttl = timestamp;
      }

       if (s.ok() && ttl > 0) {
        net::RedisCmdArgsType argv;
        std::string cmd;

        argv.push_back("EXPIRE");
        argv.push_back(key);
        argv.push_back(std::to_string(ttl));

        net::SerializeRedisCommand(argv, &cmd);
        PlusNum();
        DispatchKey(cmd, key);
      }
    }

    if (!cursor) {
      break;
    }
  }
}

void MigratorThread::MigrateDB() {
  switch (int(type_)) {
    case int(storage::DataType::kStrings) : {
      MigrateStringsDB();
      break;
    }

    case int(storage::DataType::kLists) : {
      MigrateListsDB();
      break;
    }

    case int(storage::DataType::kHashes) : {
      MigrateHashesDB();
      break;
    }

    case int(storage::DataType::kSets) : {
      MigrateSetsDB();
      break;
    }

    case int(storage::DataType::kZSets) : {
      MigrateZsetsDB();
      break;
    }

    case int(storage::DataType::kStreams) : {
      MigrateStreamsDB();
      break;
    }

    default: {
      LOG(WARNING) << "illegal db type " << type_;
      break;
    }
  }
}

void MigratorThread::DispatchKey(const std::string &command, const std::string& key) {
  thread_index_ = (thread_index_ + 1) % thread_num_;
  size_t idx = thread_index_;
  if (key.size()) { // no empty
    idx = std::hash<std::string>()(key) % thread_num_;
  }
  (*senders_)[idx]->SendRedisCommand(command);
}

const char* GetDBTypeString(int type) {
  switch (type) {
    case int(storage::DataType::kStrings) : {
	  return "storage::DataType::kStrings";
    }

    case int(storage::DataType::kLists) : {
	  return "storage::DataType::kLists";
    }

    case int(storage::DataType::kHashes) : {
	  return "storage::DataType::kHashes";
    }

    case int(storage::DataType::kSets) : {
	  return "storage::DataType::kSets";
    }

    case int(storage::DataType::kZSets) : {
	  return "storage::DataType::kZSets";
    }
    case int(storage::DataType::kStreams) : {
    return "storage::DataType::kStreams";
    }
    default: {
	  return "storage::Unknown";
    }
  }
}

void *MigratorThread::ThreadMain() {
  MigrateDB();
  should_exit_ = true;
  LOG(INFO) << GetDBTypeString(type_) << " keys have been dispatched completly";
  return NULL;
}
