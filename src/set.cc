/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "set.h"
#include <cassert>
#include "client.h"
#include "store.h"

namespace pikiwidb {

PObject PObject::CreateSet() {
  PObject set(kPTypeSet);
  set.Reset(new PSet);

  return set;
}

#define GET_SET(setname)                                         \
  PObject* value;                                                \
  PError err = PSTORE.GetValueByType(setname, value, kPTypeSet); \
  if (err != kPErrorOK) {                                        \
    if (err == kPErrorNotExist) {                                \
      FormatNull(reply);                                         \
    } else {                                                     \
      ReplyError(err, reply);                                    \
    }                                                            \
    return err;                                                  \
  }

#define GET_OR_SET_SET(setname)                                  \
  PObject* value;                                                \
  PError err = PSTORE.GetValueByType(setname, value, kPTypeSet); \
  if (err != kPErrorOK && err != kPErrorNotExist) {              \
    ReplyError(err, reply);                                      \
    return err;                                                  \
  }                                                              \
  if (err == kPErrorNotExist) {                                  \
    value = PSTORE.SetValue(setname, PObject::CreateSet());      \
  }

static bool RandomMember(const PSet& set, PString& res) {
  PSet::const_local_iterator it = RandomHashMember(set);

  if (it != PSet::const_local_iterator()) {
    res = *it;
    return true;
  }

  return false;
}

PError spop(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  PString res;
  if (RandomMember(*set, res)) {
    FormatBulk(res, reply);
    set->erase(res);
    if (set->empty()) {
      PSTORE.DeleteKey(params[1]);
    }

    std::vector<PString> translated;
    translated.push_back("srem");
    translated.push_back(params[1]);
    translated.push_back(res);

    PClient::Current()->RewriteCmd(translated);
  } else {
    FormatNull(reply);
    return kPErrorNotExist;
  }

  return kPErrorOK;
}

PError srandmember(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  PString res;
  if (RandomMember(*set, res)) {
    FormatBulk(res, reply);
  } else {
    FormatNull(reply);
  }

  return kPErrorOK;
}

PError sadd(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_OR_SET_SET(params[1]);

  int res = 0;
  auto set = value->CastSet();
  for (size_t i = 2; i < params.size(); ++i) {
    if (set->insert(params[i]).second) {
      ++res;
    }
  }

  FormatInt(res, reply);
  return kPErrorOK;
}

PError scard(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  long size = static_cast<long>(set->size());

  FormatInt(size, reply);
  return kPErrorOK;
}

PError srem(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  int res = 0;
  for (size_t i = 2; i < params.size(); ++i) {
    if (set->erase(params[i]) != 0) {
      ++res;
    }
  }

  if (set->empty()) {
    PSTORE.DeleteKey(params[1]);
  }

  FormatInt(res, reply);
  return kPErrorOK;
}

PError sismember(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  long res = static_cast<long>(set->count(params[2]));

  FormatInt(res, reply);
  return kPErrorOK;
}

PError smembers(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  PreFormatMultiBulk(set->size(), reply);
  for (const auto& member : *set) {
    FormatBulk(member, reply);
  }

  return kPErrorOK;
}

PError smove(const std::vector<PString>& params, UnboundedBuffer* reply) {
  GET_SET(params[1]);

  auto set = value->CastSet();
  int ret = static_cast<int>(set->erase(params[3]));
  if (ret != 0) {
    PObject* dst;
    err = PSTORE.GetValueByType(params[2], dst, kPTypeSet);
    if (err == kPErrorNotExist) {
      err = kPErrorOK;
      PObject val(PObject::CreateSet());
      dst = PSTORE.SetValue(params[2], std::move(val));
    }

    if (err == kPErrorOK) {
      auto dset = dst->CastSet();
      dset->insert(params[3]);
    }
  }

  FormatInt(ret, reply);
  return err;
}

PSet& PSet_diff(const PSet& l, const PSet& r, PSet& result) {
  for (const auto& le : l) {
    if (r.count(le) == 0) {
      result.insert(le);
    }
  }

  return result;
}

PSet& PSet_inter(const PSet& l, const PSet& r, PSet& result) {
  for (const auto& le : l) {
    if (r.count(le) != 0) {
      result.insert(le);
    }
  }

  return result;
}

PSet& PSet_union(const PSet& l, const PSet& r, PSet& result) {
  for (const auto& re : r) {
    result.insert(re);
  }

  for (const auto& le : l) {
    result.insert(le);
  }

  return result;
}

enum SetOperation {
  kSetOperationDiff,
  kSetOperationInter,
  kSetOperationUnion,
};

static void _set_operation(const std::vector<PString>& params, size_t offset, PSet& res, SetOperation oper) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[offset], value, kPTypeSet);
  if (err != kPErrorOK && oper != kSetOperationUnion) {
    return;
  }

  auto set = value->CastSet();
  if (set) {
    res = *set;
  }

  for (size_t i = offset + 1; i < params.size(); ++i) {
    PObject* val;
    PError err = PSTORE.GetValueByType(params[i], val, kPTypeSet);
    if (err != kPErrorOK) {
      if (oper == kSetOperationInter) {
        res.clear();
        return;
      }
      continue;
    }

    PSet tmp;
    auto r = val->CastSet();
    if (oper == kSetOperationDiff) {
      PSet_diff(res, *r, tmp);
    } else if (oper == kSetOperationInter) {
      PSet_inter(res, *r, tmp);
    } else if (oper == kSetOperationUnion) {
      PSet_union(res, *r, tmp);
    }

    res.swap(tmp);

    if (oper != kSetOperationUnion && res.empty()) {
      return;
    }
  }
}

PError sdiffstore(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject obj(PObject::CreateSet());
  auto res = obj.CastSet();
  PSTORE.SetValue(params[1], std::move(obj));

  _set_operation(params, 2, *res, kSetOperationDiff);

  FormatInt(static_cast<long>(res->size()), reply);
  return kPErrorOK;
}

PError sdiff(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PSet res;
  _set_operation(params, 1, res, kSetOperationDiff);

  PreFormatMultiBulk(res.size(), reply);
  for (const auto& elem : res) {
    FormatBulk(elem, reply);
  }

  return kPErrorOK;
}

PError sinter(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PSet res;
  _set_operation(params, 1, res, kSetOperationInter);

  PreFormatMultiBulk(res.size(), reply);
  for (const auto& elem : res) {
    FormatBulk(elem, reply);
  }

  return kPErrorOK;
}

PError sinterstore(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject obj(PObject::CreateSet());
  auto res = obj.CastSet();
  PSTORE.SetValue(params[1], std::move(obj));

  _set_operation(params, 2, *res, kSetOperationInter);

  FormatInt(static_cast<long>(res->size()), reply);
  return kPErrorOK;
}

PError sunion(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PSet res;
  _set_operation(params, 1, res, kSetOperationUnion);

  PreFormatMultiBulk(res.size(), reply);
  for (const auto& elem : res) {
    FormatBulk(elem, reply);
  }

  return kPErrorOK;
}

PError sunionstore(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject obj(PObject::CreateSet());
  auto res = obj.CastSet();
  PSTORE.SetValue(params[1], std::move(obj));

  _set_operation(params, 2, *res, kSetOperationUnion);

  FormatInt(static_cast<long>(res->size()), reply);
  return kPErrorOK;
}

size_t SScanKey(const PSet& qset, size_t cursor, size_t count, std::vector<PString>& res) {
  if (qset.empty()) {
    return 0;
  }

  std::vector<PSet::const_local_iterator> iters;
  size_t newCursor = ScanHashMember(qset, cursor, count, iters);

  res.reserve(iters.size());
  for (auto it : iters) {
    res.push_back(*it);
  }

  return newCursor;
}

}  // namespace pikiwidb
