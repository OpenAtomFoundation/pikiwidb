/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "store.h"
#include <cassert>
#include <limits>
#include "client.h"
#include "common.h"
#include "config.h"
#include "event_loop.h"
#include "leveldb.h"
#include "log.h"
#include "multi.h"
namespace pikiwidb {

uint32_t PObject::lruclock = static_cast<uint32_t>(::time(nullptr));

PObject::PObject(PType t) : type(t) {
  switch (type) {
    case PType_list:
      encoding = PEncode_list;
      break;

    case PType_set:
      encoding = PEncode_set;
      break;

    case PType_sortedSet:
      encoding = PEncode_zset;
      break;

    case PType_hash:
      encoding = PEncode_hash;
      break;

    default:
      encoding = PEncode_invalid;
      break;
  }

  lru = 0;
  value = nullptr;
}

PObject::~PObject() { freeValue(); }

void PObject::Clear() {
  freeValue();

  type = PType_invalid;
  encoding = PEncode_invalid;
  lru = 0;
  value = nullptr;
}

void PObject::Reset(void* newvalue) {
  freeValue();
  value = newvalue;
}

PObject::PObject(PObject&& obj) { moveFrom(std::move(obj)); }

PObject& PObject::operator=(PObject&& obj) {
  moveFrom(std::move(obj));
  return *this;
}

void PObject::moveFrom(PObject&& obj) {
  this->Reset();

  this->encoding = obj.encoding;
  this->type = obj.type;
  this->value = obj.value;
  this->lru = obj.lru;

  obj.encoding = PEncode_invalid;
  obj.type = PType_invalid;
  obj.value = nullptr;
  obj.lru = 0;
}

void PObject::freeValue() {
  switch (encoding) {
    case PEncode_raw:
      delete CastString();
      break;

    case PEncode_list:
      delete CastList();
      break;

    case PEncode_set:
      delete CastSet();
      break;

    case PEncode_zset:
      delete CastSortedSet();
      break;

    case PEncode_hash:
      delete CastHash();
      break;

    default:
      break;
  }
}

int PStore::dirty_ = 0;

void PStore::ExpiredDB::SetExpire(const PString& key, uint64_t when) { expireKeys_.insert_or_assign(key, when); }

int64_t PStore::ExpiredDB::TTL(const PString& key, uint64_t now) {
  if (!PSTORE.ExistsKey(key)) {
    return ExpireResult::notExist;
  }

  ExpireResult ret = ExpireIfNeed(key, now);
  switch (ret) {
    case ExpireResult::expired:
    case ExpireResult::persist:
      return ret;

    default:
      break;
  }

  auto it(expireKeys_.find(key));
  return static_cast<int64_t>(it->second - now);
}

bool PStore::ExpiredDB::ClearExpire(const PString& key) {
  return ExpireResult::expired == ExpireIfNeed(key, std::numeric_limits<uint64_t>::max());
}

PStore::ExpireResult PStore::ExpiredDB::ExpireIfNeed(const PString& key, uint64_t now) {
  auto it(expireKeys_.find(key));

  if (it != expireKeys_.end()) {
    if (it->second > now) {
      return ExpireResult::notExpire;
    }

    WARN("Delete timeout key {}", it->first);
    PSTORE.DeleteKey(it->first);
    // XXX: may throw exception if hash function crash
    expireKeys_.erase(it);
    return ExpireResult::expired;
  }

  return ExpireResult::persist;
}

int PStore::ExpiredDB::LoopCheck(uint64_t now) {
  const int kMaxDel = 100;
  const int kMaxCheck = 2000;

  int nDel = 0;
  int nLoop = 0;

  for (auto it = expireKeys_.begin(); it != expireKeys_.end() && nDel < kMaxDel && nLoop < kMaxCheck; ++nLoop) {
    if (it->second <= now) {
      // time to delete
      INFO("LoopCheck try delete key:{}", it->first);

      std::vector<PString> params{"del", it->first};
      Propagate(params);

      PSTORE.DeleteKey(it->first);
      it = expireKeys_.erase(it);

      ++nDel;
    } else {
      ++it;
    }
  }

  return nDel;
}

bool PStore::BlockedClients::BlockClient(const PString& key, PClient* client, uint64_t timeout, ListPosition pos,
                                         const PString* target) {
  if (!client->WaitFor(key, target)) {
    ERROR("{} is already waited by {}", key, client->GetName());
    return false;
  }

  auto it = blockedClients_.find(key);
  if (it != blockedClients_.end()) {
    // since folly doesn't support modify value directly, we have to make a copy to update atomically
    auto clients = it->second;
    clients.emplace_back(std::static_pointer_cast<PClient>(client->shared_from_this()), timeout, pos);
    blockedClients_.assign(key, clients);
  } else {
    std::list<std::tuple<std::weak_ptr<PClient>, uint64_t, ListPosition>> clients;
    clients.emplace_back(std::static_pointer_cast<PClient>(client->shared_from_this()), timeout, pos);
    blockedClients_.insert(key, clients);
  }

  INFO("{} is waited by {}, timeout {}", key, client->GetName(), timeout);
  return true;
}

size_t PStore::BlockedClients::UnblockClient(PClient* client) {
  size_t n = 0;
  const auto& keys = client->WaitingKeys();

  for (const auto& key : keys) {
    Clients clients = blockedClients_.find(key)->second;
    assert(!clients.empty());

    for (auto it(clients.begin()); it != clients.end(); ++it) {
      auto cli(std::get<0>(*it).lock());
      if (cli && cli.get() == client) {
        INFO("unblock {} for key {}", client->GetName(), key);
        clients.erase(it);

        ++n;
        break;
      }
    }
    blockedClients_.assign(key, clients);
  }

  client->ClearWaitingKeys();
  return n;
}

size_t PStore::BlockedClients::ServeClient(const PString& key, const PLIST& list) {
  assert(!list->empty());

  auto it = blockedClients_.find(key);
  if (it == blockedClients_.end()) {
    return 0;
  }

  Clients clients = it->second;
  if (clients.empty()) {
    return 0;
  }

  size_t nServed = 0;

  while (!list->empty() && !clients.empty()) {
    auto cli(std::get<0>(clients.front()).lock());
    auto pos(std::get<2>(clients.front()));

    if (cli) {
      bool errorTarget = false;
      const PString& target = cli->GetTarget();

      PObject* dst = nullptr;

      if (!target.empty()) {
        INFO("{} is try lpush to target list {}", list->front(), target);

        // check target list
        PError err = PSTORE.GetValueByType(target, dst, PType_list);
        if (err != PError_ok) {
          if (err != PError_notExist) {
            UnboundedBuffer reply;
            ReplyError(err, &reply);
            cli->SendPacket(reply);
            errorTarget = true;
          } else {
            dst = PSTORE.SetValue(target, PObject::CreateList());
          }
        }
      }

      if (!errorTarget) {
        if (dst) {
          auto dstlist = dst->CastList();
          dstlist->push_front(list->back());
          INFO("{} success lpush to target list {}", list->front(), target);

          std::vector<PString> params{"lpush", target, list->back()};
          Propagate(params);
        }

        UnboundedBuffer reply;

        if (!dst) {
          PreFormatMultiBulk(2, &reply);
          FormatBulk(key, &reply);
        }

        if (pos == ListPosition::head) {
          FormatBulk(list->front(), &reply);
          list->pop_front();

          std::vector<PString> params{"lpop", key};
          Propagate(params);
        } else {
          FormatBulk(list->back(), &reply);
          list->pop_back();

          std::vector<PString> params{"rpop", key};
          Propagate(params);
        }

        cli->SendPacket(reply);
        INFO("Serve client {} list key : {}", cli->GetName(), key);
      }

      UnblockClient(cli.get());
      ++nServed;
    } else {
      clients.pop_front();
      blockedClients_.assign(it->first, clients);
    }
  }

  return nServed;
}

int PStore::BlockedClients::LoopCheck(uint64_t now) {
  int n = 0;

  for (auto it(blockedClients_.begin()); it != blockedClients_.end() && n < 100;) {
    // iterator in folly::ConcurrentHashMap is always const
    Clients clients = it->second;
    for (auto cli(clients.begin()); cli != clients.end();) {
      if (std::get<1>(*cli) < now) {  // timeout
        ++n;

        const PString& key = it->first;
        auto scli(std::get<0>(*cli).lock());
        if (scli && scli->WaitingKeys().count(key)) {
          INFO("{} is timeout for waiting key {}", scli->GetName(), key);
          UnboundedBuffer reply;
          FormatNull(&reply);
          scli->SendPacket(reply);
          scli->ClearWaitingKeys();
        }

        clients.erase(cli++);
        // clients is a copy, once it's modified, map should update it
        blockedClients_.assign(it->first, clients);
      } else {
        ++cli;
      }
    }

    if (clients.empty()) {
      it = blockedClients_.erase(it);
    } else {
      ++it;
    }
  }

  return n;
}

PStore& PStore::Instance() {
  static PStore store;
  return store;
}

void PStore::Init(int dbNum) {
  if (dbNum < 1) {
    dbNum = 1;
  } else if (dbNum > kMaxDBNum) {
    dbNum = kMaxDBNum;
  }

  dbs_.resize(dbNum);
  expiredDBs_.resize(dbNum);
  blockedClients_.resize(dbNum);
}

int PStore::LoopCheckExpire(uint64_t now) { return expiredDBs_[dbno_].LoopCheck(now); }

int PStore::LoopCheckBlocked(uint64_t now) { return blockedClients_[dbno_].LoopCheck(now); }

int PStore::SelectDB(int dbno) {
  if (dbno == dbno_) {
    return dbno_;
  }

  if (dbno >= 0 && dbno < static_cast<int>(dbs_.size())) {
    int oldDB = dbno_;

    dbno_ = dbno;
    return oldDB;
  }

  return -1;
}

int PStore::GetDB() const { return dbno_; }

const PObject* PStore::GetObject(const PString& key) const {
  auto db = &dbs_[dbno_];
  PDB::const_iterator it(db->find(key));
  if (it != db->end()) {
    return &it->second;
  }

  if (!backends_.empty()) {
    // if it's in dirty list, it must be deleted, wait sync to backend
    if (waitSyncKeys_[dbno_].find(key) != waitSyncKeys_[dbno_].end()) {
      return nullptr;
    }

    // load from leveldb, if has, insert to pikiwidb cache
    PObject obj = backends_[dbno_]->Get(key);
    if (obj.type != PType_invalid) {
      DEBUG("GetKey from leveldb:{}", key);

      unsigned int remainTtlSecondsTemp = obj.lru;
      // XXX: assign lru here cause everything get from db is immutable
      obj.lru = PObject::lruclock;
      db->try_emplace(key, std::move(obj));
      const PObject& realobj = db->find(key)->second;

      // trick: use lru field to store the remain seconds to be expired.
      unsigned int remainTtlSeconds = remainTtlSecondsTemp;
      if (remainTtlSeconds > 0) {
        SetExpire(key, ::Now() + remainTtlSeconds * 1000);
      }

      return &realobj;
    }
  }

  return nullptr;
}

bool PStore::DeleteKey(const PString& key) {
  auto db = &dbs_[dbno_];
  // add to dirty queue
  if (!waitSyncKeys_.empty()) {
    waitSyncKeys_[dbno_].insert_or_assign(key, nullptr);  // null implies delete data
  }

  size_t ret = 0;
  // erase() from folly ConcurrentHashmap will throw an exception if hash function crashes
  try {
    ret = db->erase(key);
  } catch (const std::exception& e) {
    return false;
  }

  return ret != 0;
}

bool PStore::ExistsKey(const PString& key) const {
  const PObject* obj = GetObject(key);
  return obj != nullptr;
}

PType PStore::KeyType(const PString& key) const {
  const PObject* obj = GetObject(key);
  if (!obj) {
    return PType_invalid;
  }

  return PType(obj->type);
}

static bool RandomMember(const PDB& hash, PString& res, PObject** val) {
  PDB::const_iterator it = FollyRandomHashMember(hash);

  if (it != hash.cend()) {
    res = it->first;
    if (val) {
      *val = const_cast<PObject*>(&it->second);
    }
    return true;
  }

  return false;
}

PString PStore::RandomKey(PObject** val) const {
  PString res;
  if (!dbs_.empty() && !dbs_[dbno_].empty()) {
    RandomMember(dbs_[dbno_], res, val);
  }

  return res;
}

size_t PStore::ScanKey(size_t cursor, size_t count, std::vector<PString>& res) const {
  if (dbs_.empty() || dbs_[dbno_].empty()) {
    return 0;
  }

  std::vector<PString> iters;
  size_t newCursor = FollyScanHashMember(dbs_[dbno_], cursor, count, iters);

  res.reserve(iters.size());
  for (const auto& it : iters) {
    res.push_back(it);
  }

  return newCursor;
}

PError PStore::GetValue(const PString& key, PObject*& value, bool touch) {
  if (touch) {
    return GetValueByType(key, value);
  }

  return GetValueByTypeNoTouch(key, value);
}

PError PStore::GetValueByType(const PString& key, PObject*& value, PType type) {
  return getValueByType(key, value, type, true);
}

PError PStore::GetValueByTypeNoTouch(const PString& key, PObject*& value, PType type) {
  return getValueByType(key, value, type, false);
}

PError PStore::getValueByType(const PString& key, PObject*& value, PType type, bool touch) {
  if (expireIfNeed(key, ::Now()) == ExpireResult::expired) {
    return PError_notExist;
  }

  auto cobj = GetObject(key);
  if (!cobj) {
    return PError_notExist;
  }

  if (type != PType_invalid && type != PType(cobj->type)) {
    return PError_type;
  }
  value = const_cast<PObject*>(cobj);
  // Do not update if child process exists
  extern pid_t g_qdbPid;
  if (touch && g_qdbPid == -1) {
    value->lru = PObject::lruclock;
  }

  return PError_ok;
}

PObject* PStore::SetValue(const PString& key, PObject&& value) {
  auto db = &dbs_[dbno_];
  value.lru = PObject::lruclock;
  auto [realObj, status] = db->insert_or_assign(key, std::move(value));
  const PObject& obj = realObj->second;

  // put this key to sync list
  if (!waitSyncKeys_.empty()) {
    waitSyncKeys_[dbno_].insert_or_assign(key, &obj);
  }

  // XXX: any better solution without const_cast?
  return const_cast<PObject*>(&obj);
}

PError PStore::Incrby(const PString& key, int64_t value, int64_t* ret) {
  PObject* old_value = nullptr;
  auto db = &dbs_[dbno_];

  // shared when reading
  std::unique_lock<std::shared_mutex> lock(mutex_);
  PError err = getValueByType(key, old_value, PType_string);
  if (err != PError_ok) {
    return err;
  }
  char* end = nullptr;
  auto str = pikiwidb::GetDecodedString(old_value);
  int64_t ival = strtoll(str->c_str(), &end, 10);
  if (*end != 0) {
    // value is not a integer
    return PError_type;
  }

  PObject new_value;
  *ret = ival + value;
  new_value = PObject::CreateString((long)(*ret));
  new_value.lru = PObject::lruclock;
  auto [realObj, status] = db->insert_or_assign(key, std::move(new_value));
  const PObject& obj = realObj->second;

  // put this key to sync list
  if (!waitSyncKeys_.empty()) {
    waitSyncKeys_[dbno_].insert_or_assign(key, &obj);
  }

  return PError_ok;
}

PError PStore::Incrbyfloat(const PString& key, std::string value, std::string* ret) {
  PObject* old_value = nullptr;
  long double old_number = 0.00f;
  long double long_double_by = 0.00f;
  auto db = &dbs_[dbno_];

  if (StrToLongDouble(value.data(), value.size(), &long_double_by)) {
    return PError_type;
  }

  // shared when reading
  std::unique_lock<std::shared_mutex> lock(mutex_);
  PError err = getValueByType(key, old_value, PType_string);
  if (err != PError_ok) {
    return err;
  }

  auto old_number_str = pikiwidb::GetDecodedString(old_value);
  // old number to long double
  if (StrToLongDouble(old_number_str->c_str(), old_number_str->size(), &old_number)) {
    return PError_type;
  }

  std::string total_string;
  long double total = old_number + long_double_by;
  if (LongDoubleToStr(total, &total_string)) {
    return PError_overflow;
  }

  *ret = total_string;
  PObject new_value;
  new_value = PObject::CreateString(total_string);
  new_value.lru = PObject::lruclock;
  auto [realObj, status] = db->insert_or_assign(key, std::move(new_value));
  const PObject& obj = realObj->second;

  // put this key to sync list
  if (!waitSyncKeys_.empty()) {
    waitSyncKeys_[dbno_].insert_or_assign(key, &obj);
  }

  return PError_ok;
}

void PStore::SetExpire(const PString& key, uint64_t when) const { expiredDBs_[dbno_].SetExpire(key, when); }

int64_t PStore::TTL(const PString& key, uint64_t now) { return expiredDBs_[dbno_].TTL(key, now); }

bool PStore::ClearExpire(const PString& key) { return expiredDBs_[dbno_].ClearExpire(key); }

PStore::ExpireResult PStore::expireIfNeed(const PString& key, uint64_t now) {
  return expiredDBs_[dbno_].ExpireIfNeed(key, now);
}

void PStore::InitExpireTimer() {
  auto loop = EventLoop::Self();
  for (int i = 0; i < static_cast<int>(expiredDBs_.size()); ++i) {
    loop->ScheduleRepeatedly(1, [&, i]() {
      int old_db = PSTORE.SelectDB(i);
      PSTORE.LoopCheckExpire(Now());
      PSTORE.SelectDB(old_db);
    });
  }
}

void PStore::ResetDB() {
  std::vector<PDB>(dbs_.size()).swap(dbs_);
  std::vector<ExpiredDB>(expiredDBs_.size()).swap(expiredDBs_);
  std::vector<BlockedClients>(blockedClients_.size()).swap(blockedClients_);
  dbno_ = 0;
}

size_t PStore::BlockedSize() const {
  size_t s = 0;
  for (const auto& b : blockedClients_) {
    s += b.Size();
  }

  return s;
}

bool PStore::BlockClient(const PString& key, PClient* client, uint64_t timeout, ListPosition pos,
                         const PString* dstList) {
  return blockedClients_[dbno_].BlockClient(key, client, timeout, pos, dstList);
}

size_t PStore::UnblockClient(PClient* client) { return blockedClients_[dbno_].UnblockClient(client); }

size_t PStore::ServeClient(const PString& key, const PLIST& list) {
  return blockedClients_[dbno_].ServeClient(key, list);
}

void PStore::InitBlockedTimer() {
  auto loop = EventLoop::Self();
  for (int i = 0; i < static_cast<int>(blockedClients_.size()); ++i) {
    loop->ScheduleRepeatedly(3, [&, i]() {
      int old_db = PSTORE.SelectDB(i);
      PSTORE.LoopCheckBlocked(Now());
      PSTORE.SelectDB(old_db);
    });
  }
}

// allkeys-lru policy
static void EvictItems() {
  PObject::lruclock = static_cast<uint32_t>(::time(nullptr));
  PObject::lruclock &= kMaxLRUValue;

  int currentDB = PSTORE.GetDB();

  DEFER { PSTORE.SelectDB(currentDB); };

  int tryCnt = 0;
  size_t usedMem = 0;
  while (tryCnt++ < 32 && (usedMem = getMemoryInfo(VmRSS)) > g_config.maxmemory) {
    if (g_config.noeviction) {
      WARN("noeviction policy, but memory usage exceeds: {}", usedMem);
      return;
    }

    for (int dbno = 0; true; ++dbno) {
      if (PSTORE.SelectDB(dbno) == -1) {
        break;
      }

      if (PSTORE.DBSize() == 0) {
        continue;
      }

      PString evictKey;
      uint32_t choosedIdle = 0;
      for (int i = 0; i < g_config.maxmemorySamples; ++i) {
        PObject* val = nullptr;

        auto key = PSTORE.RandomKey(&val);
        if (!val) {
          continue;
        }

        auto idle = EstimateIdleTime(val->lru);
        if (evictKey.empty() || choosedIdle < idle) {
          evictKey = std::move(key);
          choosedIdle = idle;
        }
      }

      if (!evictKey.empty()) {
        PSTORE.DeleteKey(evictKey);
        WARN("Evict '{}' in db {}, idle time: {}, used mem: {}", evictKey, dbno, choosedIdle, usedMem);
      }
    }
  }
}

uint32_t EstimateIdleTime(uint32_t lru) {
  if (lru <= PObject::lruclock) {
    return PObject::lruclock - lru;
  } else {
    return (kMaxLRUValue - lru) + PObject::lruclock;
  }
}

void PStore::InitEvictionTimer() {
  auto loop = EventLoop::Self();
  // emit eviction every second.
  loop->ScheduleRepeatedly(1000, EvictItems);
}

void PStore::InitDumpBackends() {
  assert(waitSyncKeys_.empty());

  if (g_config.backend == kBackEndNone) {
    return;
  }

  if (g_config.backend == kBackEndLeveldb) {
    waitSyncKeys_.resize(dbs_.size());
    for (size_t i = 0; i < dbs_.size(); ++i) {
      std::unique_ptr<PLeveldb> db(new PLeveldb);
      PString dbpath = g_config.backendPath + std::to_string(i);
      if (!db->Open(dbpath.data())) {
        assert(false);
      } else {
        INFO("Open leveldb {}", dbpath);
      }

      backends_.push_back(std::move(db));
    }
  } else {
    // ERROR: unsupport backend
    return;
  }

  auto loop = EventLoop::Self();
  for (int i = 0; i < static_cast<int>(backends_.size()); ++i) {
    loop->ScheduleRepeatedly(1000 / g_config.backendHz, [&, i]() {
      int old_db = PSTORE.SelectDB(i);
      PSTORE.DumpToBackends(i);
      PSTORE.SelectDB(old_db);
    });
  }
}

void PStore::DumpToBackends(int dbno) {
  if (static_cast<int>(waitSyncKeys_.size()) <= dbno) {
    return;
  }

  const int kMaxSync = 100;
  int processed = 0;
  auto& dirtyKeys = waitSyncKeys_[dbno];

  uint64_t now = ::Now();
  for (auto it = dirtyKeys.begin(); processed++ < kMaxSync && it != dirtyKeys.end();) {
    // check ttl
    int64_t when = PSTORE.TTL(it->first, now);

    if (it->second && when != PStore::ExpireResult::expired) {
      assert(when != PStore::ExpireResult::notExpire);

      if (when > 0) {
        when += now;
      }

      backends_[dbno]->Put(it->first, *it->second, when);
      DEBUG("UPDATE leveldb key {}, when = {}", it->first, when);
    } else {
      backends_[dbno]->Delete(it->first);
      DEBUG("DELETE leveldb key {}", it->first);
    }

    it = dirtyKeys.erase(it);
  }
}

void PStore::AddDirtyKey(const PString& key) {
  // put this key to sync list
  if (!waitSyncKeys_.empty()) {
    PObject* obj = nullptr;
    GetValue(key, obj);
    waitSyncKeys_[dbno_].insert_or_assign(key, obj);
  }
}

void PStore::AddDirtyKey(const PString& key, const PObject* value) {
  // put this key to sync list
  if (!waitSyncKeys_.empty()) {
    waitSyncKeys_[dbno_].insert_or_assign(key, value);
  }
}

std::vector<PString> g_dirtyKeys;

void Propagate(const std::vector<PString>& params) {
  assert(!params.empty());

  if (!g_dirtyKeys.empty()) {
    for (const auto& k : g_dirtyKeys) {
      ++PStore::dirty_;
      PMulti::Instance().NotifyDirty(PSTORE.GetDB(), k);

      PSTORE.AddDirtyKey(k);  // TODO optimize
    }
    g_dirtyKeys.clear();
  } else if (params.size() > 1) {
    ++PStore::dirty_;
    PMulti::Instance().NotifyDirty(PSTORE.GetDB(), params[1]);
    PSTORE.AddDirtyKey(params[1]);  // TODO optimize
  }

  PREPL.SendToSlaves(params);
}

void Propagate(int dbno, const std::vector<PString>& params) {
  PMulti::Instance().NotifyDirtyAll(dbno);
  Propagate(params);
}

}  // namespace pikiwidb
