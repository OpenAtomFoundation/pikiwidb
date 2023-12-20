/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "pstring.h"
#include <cassert>
#include "log.h"
#include "store.h"

namespace pikiwidb {

PObject PObject::CreateString(const PString& value) {
  PObject obj(kPTypeString);

  long val;
  if (IsValidNumber(value.c_str())) {
    Strtol(value.c_str(), value.size(), &val);
    obj.encoding = kPEncodeInt;
    obj.value = (void*)val;
    DEBUG("set long value {}", val);
  } else {
    obj.encoding = kPEncodeRaw;
    obj.value = new PString(value);
  }

  return obj;
}

PObject PObject::CreateString(long val) {
  PObject obj(kPTypeString);

  obj.encoding = kPEncodeInt;
  obj.value = (void*)val;

  return obj;
}

static void DeleteString(PString* s) { delete s; }

static void NotDeleteString(PString*) {}

std::unique_ptr<PString, void (*)(PString*)> GetDecodedString(const PObject* value) {
  if (value->encoding == kPEncodeRaw) {
    return std::unique_ptr<PString, void (*)(PString*)>(value->CastString(), NotDeleteString);
  } else if (value->encoding == kPEncodeInt) {
    intptr_t val = (intptr_t)value->value;

    char vbuf[32];
    snprintf(vbuf, sizeof vbuf - 1, "%ld", val);
    return std::unique_ptr<PString, void (*)(PString*)>(new PString(vbuf), DeleteString);
  } else {
    assert(!!!"error string encoding");
  }

  return std::unique_ptr<PString, void (*)(PString*)>(nullptr, NotDeleteString);
}

static bool SetValue(const PString& key, const PString& value, bool exclusive = false) {
  if (exclusive) {
    PObject* val;
    if (PSTORE.GetValue(key, val) == kPErrorOK) {
      return false;
    }
  }

  PSTORE.ClearExpire(key);  // clear key's old ttl
  PSTORE.SetValue(key, PObject::CreateString(value));

  return true;
}

PError set(const std::vector<PString>& params, UnboundedBuffer* reply) {
  SetValue(params[1], params[2]);
  FormatOK(reply);
  return kPErrorOK;
}

PError setnx(const std::vector<PString>& params, UnboundedBuffer* reply) {
  if (SetValue(params[1], params[2], true)) {
    Format1(reply);
  } else {
    Format0(reply);
  }

  return kPErrorOK;
}

PError mset(const std::vector<PString>& params, UnboundedBuffer* reply) {
  if (params.size() % 2 != 1) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  for (size_t i = 1; i < params.size(); i += 2) {
    g_dirtyKeys.push_back(params[i]);
    SetValue(params[i], params[i + 1]);
  }

  FormatOK(reply);
  return kPErrorOK;
}

PError msetnx(const std::vector<PString>& params, UnboundedBuffer* reply) {
  if (params.size() % 2 != 1) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  for (size_t i = 1; i < params.size(); i += 2) {
    PObject* val;
    if (PSTORE.GetValue(params[i], val) == kPErrorOK) {
      Format0(reply);
      return kPErrorOK;
    }
  }

  for (size_t i = 1; i < params.size(); i += 2) {
    g_dirtyKeys.push_back(params[i]);
    SetValue(params[i], params[i + 1]);
  }

  Format1(reply);
  return kPErrorOK;
}

PError setex(const std::vector<PString>& params, UnboundedBuffer* reply) {
  long seconds;
  if (!Strtol(params[2].c_str(), params[2].size(), &seconds)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  const auto& key = params[1];
  PSTORE.SetValue(key, PObject::CreateString(params[3]));
  PSTORE.SetExpire(key, ::Now() + seconds * 1000);

  FormatOK(reply);
  return kPErrorOK;
}

PError psetex(const std::vector<PString>& params, UnboundedBuffer* reply) {
  long milliseconds;
  if (!Strtol(params[2].c_str(), params[2].size(), &milliseconds)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  const auto& key = params[1];
  PSTORE.SetValue(key, PObject::CreateString(params[3]));
  PSTORE.SetExpire(key, ::Now() + milliseconds);

  FormatOK(reply);
  return kPErrorOK;
}

PError setrange(const std::vector<PString>& params, UnboundedBuffer* reply) {
  long offset;
  if (!Strtol(params[2].c_str(), params[2].size(), &offset)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      value = PSTORE.SetValue(params[1], PObject::CreateString(""));
    } else {
      ReplyError(err, reply);
      return err;
    }
  }

  auto str = GetDecodedString(value);
  const size_t newSize = offset + params[3].size();

  if (newSize > str->size()) {
    str->resize(newSize, '\0');
  }
  str->replace(offset, params[3].size(), params[3]);

  if (value->encoding == kPEncodeInt) {
    value->Reset(new PString(*str));
    value->encoding = kPEncodeRaw;
  }

  FormatInt(static_cast<long>(str->size()), reply);
  return kPErrorOK;
}

static void AddReply(PObject* value, UnboundedBuffer* reply) {
  auto str = GetDecodedString(value);
  FormatBulk(str->c_str(), str->size(), reply);
}

PError get(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      FormatNull(reply);
    } else {
      ReplyError(err, reply);
    }

    return err;
  }

  AddReply(value, reply);
  return kPErrorOK;
}

PError mget(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PreFormatMultiBulk(params.size() - 1, reply);
  for (size_t i = 1; i < params.size(); ++i) {
    PObject* value;
    PError err = PSTORE.GetValueByType(params[i], value, kPTypeString);
    if (err != kPErrorOK) {
      FormatNull(reply);
    } else {
      AddReply(value, reply);
    }
  }

  return kPErrorOK;
}

PError getrange(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err != kPErrorOK) {
    if (err == kPErrorNotExist) {
      FormatBulk("", 0, reply);
    } else {
      ReplyError(err, reply);
    }

    return err;
  }

  long start = 0, end = 0;
  if (!Strtol(params[2].c_str(), params[2].size(), &start) || !Strtol(params[3].c_str(), params[3].size(), &end)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  auto str = GetDecodedString(value);
  AdjustIndex(start, end, str->size());

  if (start <= end) {
    FormatBulk(&(*str)[start], end - start + 1, reply);
  } else {
    FormatEmptyBulk(reply);
  }

  return kPErrorOK;
}

PError getset(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value = nullptr;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);

  switch (err) {
    case kPErrorNotExist:
      // fall through

    case kPErrorOK:
      if (!value) {
        FormatNull(reply);
      } else {
        FormatBulk(*GetDecodedString(value), reply);
      }

      PSTORE.SetValue(params[1], PObject::CreateString(params[2]));
      break;

    default:
      ReplyError(err, reply);
      return err;
  }

  return kPErrorOK;
}

PError append(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);

  switch (err) {
    case kPErrorOK: {
      auto s = GetDecodedString(value);
      value = PSTORE.SetValue(params[1], PObject::CreateString(*s + params[2]));
    } break;

    case kPErrorNotExist:
      value = PSTORE.SetValue(params[1], PObject::CreateString(params[2]));
      break;

    default:
      ReplyError(err, reply);
      return err;
  };

  auto s = GetDecodedString(value);
  FormatInt(static_cast<long>(s->size()), reply);
  return kPErrorOK;
}

PError bitcount(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err != kPErrorOK) {
    if (err == kPErrorType) {
      ReplyError(kPErrorType, reply);
    } else {
      Format0(reply);
    }

    return kPErrorOK;
  }

  if (params.size() != 2 && params.size() != 4) {
    ReplyError(kPErrorParam, reply);
    return kPErrorParam;
  }

  long start = 0;
  long end = -1;
  if (params.size() == 4) {
    if (!Strtol(params[2].c_str(), params[2].size(), &start) || !Strtol(params[3].c_str(), params[3].size(), &end)) {
      ReplyError(kPErrorNan, reply);
      return kPErrorNan;
    }
  }

  auto str = GetDecodedString(value);
  AdjustIndex(start, end, str->size());

  size_t cnt = 0;
  if (end >= start) {
    cnt = BitCount((const uint8_t*)str->data() + start, end - start + 1);
  }

  FormatInt(static_cast<long>(cnt), reply);
  return kPErrorOK;
}

PError getbit(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err != kPErrorOK) {
    Format0(reply);
    return kPErrorOK;
  }

  long offset = 0;
  if (!Strtol(params[2].c_str(), params[2].size(), &offset)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  auto str = GetDecodedString(value);
  const uint8_t* buf = (const uint8_t*)str->c_str();
  size_t size = 8 * str->size();

  if (offset < 0 || offset >= static_cast<long>(size)) {
    Format0(reply);
    return kPErrorOK;
  }

  size_t bytesOffset = offset / 8;
  size_t bitsOffset = offset % 8;
  uint8_t byte = buf[bytesOffset];
  if (byte & (0x1 << bitsOffset)) {
    Format1(reply);
  } else {
    Format0(reply);
  }

  return kPErrorOK;
}

PError setbit(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(params[1], value, kPTypeString);
  if (err == kPErrorNotExist) {
    value = PSTORE.SetValue(params[1], PObject::CreateString(""));
    err = kPErrorOK;
  }

  if (err != kPErrorOK) {
    Format0(reply);
    return err;
  }

  long offset = 0;
  long on = 0;
  if (!Strtol(params[2].c_str(), params[2].size(), &offset) || !Strtol(params[3].c_str(), params[3].size(), &on)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  if (offset < 0 || offset > kStringMaxBytes) {
    Format0(reply);
    return kPErrorOK;
  }

  PString newVal(*GetDecodedString(value));

  size_t bytes = offset / 8;
  size_t bits = offset % 8;

  if (bytes + 1 > newVal.size()) {
    newVal.resize(bytes + 1, '\0');
  }

  const char oldByte = newVal[bytes];
  char& byte = newVal[bytes];
  if (on) {
    byte |= (0x1 << bits);
  } else {
    byte &= ~(0x1 << bits);
  }

  value->Reset(new PString(newVal));
  value->encoding = kPEncodeRaw;
  FormatInt((oldByte & (0x1 << bits)) ? 1 : 0, reply);

  return kPErrorOK;
}

static PError ChangeFloatValue(const PString& key, float delta, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(key, value, kPTypeString);
  if (err == kPErrorNotExist) {
    value = PSTORE.SetValue(key, PObject::CreateString("0"));
    err = kPErrorOK;
  }

  if (err != kPErrorOK) {
    ReplyError(err, reply);
    return err;
  }

  auto val = GetDecodedString(value);
  float oldVal = 0;
  if (!Strtof(val->c_str(), val->size(), &oldVal)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  char newVal[32];
  int len = snprintf(newVal, sizeof newVal - 1, "%.6g", (oldVal + delta));
  value->Reset(new PString(newVal, len));
  value->encoding = kPEncodeRaw;

  FormatBulk(newVal, len, reply);
  return kPErrorOK;
}

PError incrbyfloat(const std::vector<PString>& params, UnboundedBuffer* reply) {
  float delta = 0;
  if (!Strtof(params[2].c_str(), params[2].size(), &delta)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  return ChangeFloatValue(params[1], delta, reply);
}

static PError ChangeIntValue(const PString& key, long delta, UnboundedBuffer* reply) {
  PObject* value;
  PError err = PSTORE.GetValueByType(key, value, kPTypeString);
  if (err == kPErrorNotExist) {
    value = PSTORE.SetValue(key, PObject::CreateString(0));
    err = kPErrorOK;
  }

  if (err != kPErrorOK) {
    ReplyError(err, reply);
    return err;
  }

  if (value->encoding != kPEncodeInt) {
    ReplyError(kPErrorNan, reply);
    return kPErrorOK;
  }

  intptr_t oldVal = (intptr_t)value->value;
  value->Reset((void*)(oldVal + delta));

  FormatInt(oldVal + delta, reply);
  return kPErrorOK;
}

PError incr(const std::vector<PString>& params, UnboundedBuffer* reply) { return ChangeIntValue(params[1], 1, reply); }
PError decr(const std::vector<PString>& params, UnboundedBuffer* reply) { return ChangeIntValue(params[1], -1, reply); }

PError incrby(const std::vector<PString>& params, UnboundedBuffer* reply) {
  long delta = 0;
  if (!Strtol(params[2].c_str(), params[2].size(), &delta)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  return ChangeIntValue(params[1], delta, reply);
}

PError decrby(const std::vector<PString>& params, UnboundedBuffer* reply) {
  long delta = 0;
  if (!Strtol(params[2].c_str(), params[2].size(), &delta)) {
    ReplyError(kPErrorNan, reply);
    return kPErrorNan;
  }

  return ChangeIntValue(params[1], -delta, reply);
}

PError strlen(const std::vector<PString>& params, UnboundedBuffer* reply) {
  PObject* val;
  PError err = PSTORE.GetValueByType(params[1], val, kPTypeString);
  if (err != kPErrorOK) {
    Format0(reply);
    return err;
  }

  auto str = GetDecodedString(val);
  FormatInt(static_cast<long>(str->size()), reply);
  return kPErrorOK;
}

enum BitOp {
  kBitOpAnd,
  kBitOpOr,
  kBitOpNot,
  kBitOpXor,
};

static PString StringBitOp(const std::vector<const PString*>& keys, BitOp op) {
  PString res;

  switch (op) {
    case kBitOpAnd:
    case kBitOpOr:
    case kBitOpXor:
      for (auto k : keys) {
        PObject* val;
        if (PSTORE.GetValueByType(*k, val, kPTypeString) != kPErrorOK) {
          continue;
        }

        auto str = GetDecodedString(val);
        if (res.empty()) {
          res = *str;
          continue;
        }

        if (str->size() > res.size()) {
          res.resize(str->size());
        }

        for (size_t i = 0; i < str->size(); ++i) {
          if (op == kBitOpAnd) {
            res[i] &= (*str)[i];
          } else if (op == kBitOpOr) {
            res[i] |= (*str)[i];
          } else if (op == kBitOpXor) {
            res[i] ^= (*str)[i];
          }
        }
      }
      break;

    case kBitOpNot: {
      assert(keys.size() == 1);
      PObject* val;
      if (PSTORE.GetValueByType(*keys[0], val, kPTypeString) != kPErrorOK) {
        break;
      }

      auto str = GetDecodedString(val);
      res.resize(str->size());

      for (size_t i = 0; i < str->size(); ++i) {
        res[i] = ~(*str)[i];
      }

      break;
    }

    default:
      break;
  }

  return res;
}

PError bitop(const std::vector<PString>& params, UnboundedBuffer* reply) {
  std::vector<const PString*> keys;
  for (size_t i = 3; i < params.size(); ++i) {
    keys.push_back(&params[i]);
  }

  PError err = kPErrorParam;
  PString res;
  if (params[1].size() == 2) {
    if (strncasecmp(params[1].c_str(), "or", 2) == 0) {
      err = kPErrorOK;
      res = StringBitOp(keys, kBitOpOr);
    }
  } else if (params[1].size() == 3) {
    if (strncasecmp(params[1].c_str(), "xor", 3) == 0) {
      err = kPErrorOK;
      res = StringBitOp(keys, kBitOpXor);
    } else if (strncasecmp(params[1].c_str(), "and", 3) == 0) {
      err = kPErrorOK;
      res = StringBitOp(keys, kBitOpAnd);
    } else if (strncasecmp(params[1].c_str(), "not", 3) == 0) {
      if (params.size() == 4) {
        err = kPErrorOK;
        res = StringBitOp(keys, kBitOpNot);
      }
    } else {
      ;
    }
  }

  if (err != kPErrorOK) {
    ReplyError(err, reply);
  } else {
    PSTORE.SetValue(params[2], PObject::CreateString(res));
    FormatInt(static_cast<long>(res.size()), reply);
  }

  return kPErrorOK;
}

}  // namespace pikiwidb
