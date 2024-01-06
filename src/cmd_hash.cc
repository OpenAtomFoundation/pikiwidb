/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "cmd_hash.h"

#include <config.h>

#include "pstd/pstd_string.h"
#include "store.h"

namespace pikiwidb {

HSetCmd::HSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryHash) {}

bool HSetCmd::DoInitial(PClient* client) {
  if (client->argv_.size() % 2 != 0) {
    client->SetRes(CmdRes::kWrongNum, kCmdNameHSet);
    return false;
  }
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  return true;
}

void HSetCmd::DoCmd(PClient* client) {
  int32_t ret = 0;
  storage::Status s;
  auto fvs = client->Fvs();

  for (size_t i = 2; i < client->argv_.size(); i += 2) {
    auto field = client->argv_[i];
    auto value = client->argv_[i + 1];
    int32_t temp = 0;
    // TODO(century): current bw doesn't support multiple fvs, fix it when necessary
    s = PSTORE.GetBackend()->HSet(client->Key(), field, value, &temp);
    if (s.ok()) {
      ret += temp;
    } else {
      // FIXME(century): need txn, if bw crashes, it should rollback
      client->SetRes(CmdRes::kErrOther);
      return;
    }
  }

  client->AppendInteger(ret);
}

HGetCmd::HGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetCmd::DoCmd(PClient* client) {
  PString value;
  auto field = client->argv_[2];
  storage::Status s = PSTORE.GetBackend()->HGet(client->Key(), field, &value);
  if (s.ok()) {
    client->AppendString(value);
  } else if (s.IsNotFound()) {
    client->AppendString("");
  } else {
    client->SetRes(CmdRes::kSyntaxErr, "hget cmd error");
  }
}

HMSetCmd::HMSetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsWrite, kAclCategoryWrite | kAclCategoryHash) {}

bool HMSetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  client->ClearFvs();
  // set fvs
  for (size_t index = 2; index < client->argv_.size(); index += 2) {
    client->Fvs().push_back({client->argv_[index], client->argv_[index + 1]});
  }
  return true;
}

void HMSetCmd::DoCmd(PClient* client) {
  storage::Status s = PSTORE.GetBackend()->HMSet(client->Key(), client->Fvs());
  if (s.ok()) {
    client->SetRes(CmdRes::kOK);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

HMGetCmd::HMGetCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HMGetCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  for (size_t i = 2; i < client->argv_.size(); ++i) {
    client->Fields().push_back(client->argv_[i]);
  }
  return true;
}

void HMGetCmd::DoCmd(PClient* client) {
  std::vector<storage::ValueStatus> vss;
  auto s = PSTORE.GetBackend()->HMGet(client->Key(), client->Fields(), &vss);
  if (s.ok() || s.IsNotFound()) {
    client->AppendArrayLenUint64(vss.size());
    for (size_t i = 0; i < vss.size(); ++i) {
      if (vss[i].status.ok()) {
        client->AppendString(vss[i].value);
      } else {
        client->AppendString("");
      }
    }
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

HGetAllCmd::HGetAllCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HGetAllCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HGetAllCmd::DoCmd(PClient* client) {
  int64_t total_fv = 0;
  int64_t cursor = 0;
  int64_t next_cursor = 0;
  size_t raw_limit = g_config.max_client_response_size;
  std::string raw;
  std::vector<storage::FieldValue> fvs;
  storage::Status s;

  do {
    fvs.clear();
    s = PSTORE.GetBackend()->HScan(client->Key(), cursor, "*", PIKIWIDB_SCAN_STEP_LENGTH, &fvs, &next_cursor);
    if (!s.ok()) {
      raw.clear();
      total_fv = 0;
      if (s.IsNotFound()) {
        client->AppendArrayLen(0);
      } else {
        client->SetRes(CmdRes::kErrOther, s.ToString());
      }
      break;
    } else {
      client->AppendArrayLenUint64(fvs.size() * 2);
      for (const auto& fv : fvs) {
        client->AppendStringLenUint64(fv.field.size());
        client->AppendContent(fv.field);
        client->AppendStringLenUint64(fv.value.size());
        client->AppendContent(fv.value);
      }
      if (raw.size() >= raw_limit) {
        client->SetRes(CmdRes::kErrOther, "Response exceeds the max-client-response-size limit");
        return;
      }
      total_fv += static_cast<int64_t>(fvs.size());
      cursor = next_cursor;
    }
  } while (cursor != 0);
}

HKeysCmd::HKeysCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HKeysCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HKeysCmd::DoCmd(PClient* client) {
  std::vector<std::string> fields;
  auto s = PSTORE.GetBackend()->HKeys(client->Key(), &fields);
  if (s.ok() || s.IsNotFound()) {
    client->AppendArrayLenUint64(fields.size());
    for (const auto& field : fields) {
      client->AppendStringLenUint64(field.size());
      client->AppendContent(field);
    }
    // update fields
    client->Fields() = std::move(fields);
  } else {
    client->SetRes(CmdRes::kErrOther, s.ToString());
  }
}

HLenCmd::HLenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HLenCmd::DoCmd(PClient* client) {
  int32_t len = 0;
  auto s = PSTORE.GetBackend()->HLen(client->Key(), &len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(len);
  } else {
    client->SetRes(CmdRes::kErrOther, "something wrong in hlen");
  }
}

HStrLenCmd::HStrLenCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HStrLenCmd::DoInitial(PClient* client) {
  client->SetKey(client->argv_[1]);
  return true;
}

void HStrLenCmd::DoCmd(PClient* client) {
  int32_t len = 0;
  auto s = PSTORE.GetBackend()->HStrlen(client->Key(), client->argv_[2], &len);
  if (s.ok() || s.IsNotFound()) {
    client->AppendInteger(len);
  } else {
    client->SetRes(CmdRes::kErrOther, "something wrong in hstrlen");
  }
}

HRandFieldCmd::HRandFieldCmd(const std::string& name, int16_t arity)
    : BaseCmd(name, arity, kCmdFlagsReadonly, kAclCategoryRead | kAclCategoryHash) {}

bool HRandFieldCmd::DoInitial(PClient* client) {
  /*
   * There should not be quantity detection here,
   * because the quantity detection of redis is after the COUNT integer detection.
   */
  // if (client->argv_.size() > 4) {
  //   client->SetRes(CmdRes::kSyntaxErr);
  //   return false;
  // }
  client->SetKey(client->argv_[1]);
  return true;
}

void HRandFieldCmd::DoCmd(PClient* client) {
  // parse arguments
  const auto& argv = client->argv_;
  int64_t count{1};
  bool with_values{false};
  if (argv.size() > 2) {
    // redis checks the integer argument first and then the number of parameters
    if (pstd::String2int(argv[2], &count) == 0) {
      client->SetRes(CmdRes::kInvalidInt);
      return;
    }
    if (argv.size() > 4) {
      client->SetRes(CmdRes::kSyntaxErr);
      return;
    }
    if (argv.size() > 3) {
      if (kWithValueString != pstd::StringToLower(argv[3])) {
        client->SetRes(CmdRes::kSyntaxErr);
        return;
      }
      with_values = true;
    }
  }

  // execute command
  std::vector<storage::FieldValue> fvs;
  auto s = PSTORE.GetBackend()->HRandField(client->Key(), count, &fvs);
  if (s.IsNotFound()) {
    client->AppendString("");
    return;
  }
  if (!s.ok()) {
    client->SetRes(CmdRes::kErrOther, s.ToString());
    return;
  }

  // reply to client
  if (argv.size() > 2) {
    client->AppendArrayLenUint64(with_values ? fvs.size() * 2 : fvs.size());
  }
  for (const auto& [field, value] : fvs) {
    client->AppendString(field);
    if (with_values) {
      client->AppendString(value);
    }
  }
}

}  // namespace pikiwidb
