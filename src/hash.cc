/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "hash.h"
#include <cassert>
#include "store.h"

namespace pikiwidb {

PObject PObject::CreateHash(std::vector<storage::FieldValue>* fvs) {
  PObject obj(kPTypeHash);
  obj.Reset(new PHash);

  if (fvs) {
    auto value = obj.CastHash();
    for (auto it = fvs->begin(); it != fvs->end(); it++) {
      value->insert(std::make_pair(it->field, it->value));
    }
  }

  return obj;
}
/*
#define GET_HASH(hashname)                                         \
  PObject* value;                                                  \
  PError err = PSTORE.GetValueByType(hashname, value, kPTypeHash); \
  if (err != kPErrorOK) {                                          \
    ReplyError(err, reply);                                        \
    return err;                                                    \
  }

#define GET_OR_SET_HASH(hashname)                                  \
  PObject* value;                                                  \
  PError err = PSTORE.GetValueByType(hashname, value, kPTypeHash); \
  if (err != kPErrorOK && err != kPErrorNotExist) {                \
    ReplyError(err, reply);                                        \
    return err;                                                    \
  }                                                                \
  if (err == kPErrorNotExist) {                                    \
    value = PSTORE.SetValue(hashname, PObject::CreateHash());      \
  }

PHash::iterator _set_hash_force(PHash& hash, const PString& key, const PString& val) {
  auto it(hash.find(key));
  if (it != hash.end()) {
    it->second = val;
  } else {
    it = hash.insert(PHash::value_type(key, val)).first;
  }

  return it;
}

bool _set_hash_if_notexist(PHash& hash, const PString& key, const PString& val) {
  return hash.insert(PHash::value_type(key, val)).second;
}

PError hset(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_OR_SET_HASH(params[1]);

  auto hash = value->CastHash();
  _set_hash_force(*hash, params[2], params[3]);

  FormatInt(1, reply);
  return kPErrorOK;
}

PError hmset(const std::vector<PString>& params, UnboundedBuffer* reply) {
  if (params.size() % 2 != 0) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  GET_OR_SET_HASH(params[1]);

  auto hash = value->CastHash();
  for (size_t i = 2; i < params.size(); i += 2) {
    _set_hash_force(*hash, params[i], params[i + 1]);
  }

  FormatOK(reply);
  return kPErrorOK;
}

PError hget(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  auto it = hash->find(params[2]);

  if (it != hash->end()) {
    FormatBulk(it->second, reply);
  } else {
    FormatNull(reply);
  }

  return kPErrorOK;
}

PError hmget(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  PreFormatMultiBulk(params.size() - 2, reply);

  auto hash = value->CastHash();
  for (size_t i = 2; i < params.size(); ++i) {
    auto it = hash->find(params[i]);
    if (it != hash->end()) {
      FormatBulk(it->second, reply);
    } else {
      FormatNull(reply);
    }
  }

  return kPErrorOK;
}

PError hgetall(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  PreFormatMultiBulk(2 * hash->size(), reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, reply);
    FormatBulk(kv.second, reply);
  }

  return kPErrorOK;
}

PError hkeys(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  PreFormatMultiBulk(hash->size(), reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.first, reply);
  }

  return kPErrorOK;
}

PError hvals(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  PreFormatMultiBulk(hash->size(), reply);

  for (const auto& kv : *hash) {
    FormatBulk(kv.second, reply);
  }

  return kPErrorOK;
}

PError hdel(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeHash);
  if (err != kPErrorOK) {
    ReplyError(err, reply);
    return err;
  }

  int del = 0;
  auto hash = value->CastHash();
  for (size_t i = 2; i < params.size(); ++i) {
    auto it = hash->find(params[i]);

    if (it != hash->end()) {
      hash->erase(it);
      ++del;
    }
  }

  FormatInt(del, reply);
  return kPErrorOK;
}

PError hexists(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  auto it = hash->find(params[2]);

  if (it != hash->end()) {
    FormatInt(1, reply);
  } else {
    FormatInt(0, reply);
  }

  return kPErrorOK;
}

PError hlen(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_HASH(params[1]);

  auto hash = value->CastHash();
  FormatInt(hash->size(), reply);
  return kPErrorOK;
}

PError hincrby(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_OR_SET_HASH(params[1]);

  auto hash = value->CastHash();
  long val = 0;
  PString* str = nullptr;
  auto it(hash->find(params[2]));
  if (it != hash->end()) {
    str = &it->second;
    if (Strtol(str->c_str(), static_cast<int>(str->size()), &val)) {
      val += atoi(params[3].c_str());
    } else {
      ReplyError(kPErrorNan, reply);
      return kPErrorNan;
    }
  } else {
    val = atoi(params[3].c_str());
    auto it = _set_hash_force(*hash, params[2], "");
    str = &it->second;
  }

  char tmp[32];
  snprintf(tmp, sizeof tmp - 1, "%ld", val);
  *str = tmp;

  FormatInt(val, reply);
  return kPErrorOK;
}

PError hincrbyfloat(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_OR_SET_HASH(params[1]);

  auto hash = value->CastHash();
  float val = 0;
  PString* str = 0;
  auto it(hash->find(params[2]));
  if (it != hash->end()) {
    str = &it->second;
    if (Strtof(str->c_str(), static_cast<int>(str->size()), &val)) {
      val += atof(params[3].c_str());
    } else {
      ReplyError(kPErrorParam, reply);
      return kPErrorParam;
    }
  } else {
    val = atof(params[3].c_str());
    auto it = _set_hash_force(*hash, params[2], "");
    str = &it->second;
  }

  char tmp[32];
  snprintf(tmp, sizeof tmp - 1, "%f", val);
  *str = tmp;

  FormatBulk(*str, reply);
  return kPErrorOK;
}

PError hsetnx(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_OR_SET_HASH(params[1]);

  auto hash = value->CastHash();
  if (_set_hash_if_notexist(*hash, params[2], params[3])) {
    FormatInt(1, reply);
  } else {
    FormatInt(0, reply);
  }

  return kPErrorOK;
}

PError hstrlen(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeHash);
  if (err != kPErrorOK) {
    Format0(reply);
    return err;
  }

  auto hash = value->CastHash();
  auto it = hash->find(params[2]);
  if (it == hash->end()) {
    Format0(reply);
  } else {
    FormatInt(static_cast<long>(it->second.size()), reply);
  }

  return kPErrorOK;
}

size_t HScanKey(const PHash& hash, size_t cursor, size_t count, std::vector<PString>& res) {
  if (hash.empty()) {
    return 0;
  }

  std::vector<PHash::const_local_iterator> iters;
  size_t newCursor = ScanHashMember(hash, cursor, count, iters);

  res.reserve(iters.size());
  for (auto it : iters) {
    res.push_back(it->first), res.push_back(it->second);
  }

  return newCursor;
}
*/
}  // namespace pikiwidb
