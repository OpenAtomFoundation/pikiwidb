/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "log.h"

#include <algorithm>
#include <memory>

#include "client.h"
#include "command.h"
#include "config.h"
#include "pikiwidb.h"
#include "pstd_string.h"
#include "slow_log.h"
#include "store.h"

namespace pikiwidb {

void CmdRes::RedisAppendLen(std::string& str, int64_t ori, const std::string& prefix) {
  str.append(prefix);
  str.append(pstd::Int2string(ori));
  str.append(CRLF);
}

void CmdRes::AppendStringVector(const std::vector<std::string>& strArray) {
  if (strArray.empty()) {
    AppendArrayLen(-1);
    return;
  }
  AppendArrayLen(static_cast<int64_t>(strArray.size()));
  for (const auto& item : strArray) {
    AppendString(item);
  }
}

void CmdRes::AppendString(const std::string& value) {
  if (value.empty()) {
    AppendStringLen(-1);
  } else {
    AppendStringLen(static_cast<int64_t>(value.size()));
    AppendContent(value);
  }
}

void CmdRes::SetRes(CmdRes::CmdRet _ret, const std::string& content) {
  ret_ = _ret;
  switch (ret_) {
    case kOk:
      SetLineString("+OK");
      break;
    case kPong:
      SetLineString("+PONG");
      break;
    case kSyntaxErr:
      SetLineString("-ERR syntax error");
      break;
    case kInvalidInt:
      SetLineString("-ERR value is not an integer or out of range");
      break;
    case kInvalidBitInt:
      SetLineString("-ERR bit is not an integer or out of range");
      break;
    case kInvalidBitOffsetInt:
      SetLineString("-ERR bit offset is not an integer or out of range");
      break;
    case kWrongBitOpNotNum:
      SetLineString("-ERR BITOP NOT must be called with a single source key.");
      break;
    case kInvalidBitPosArgument:
      SetLineString("-ERR The bit argument must be 1 or 0.");
      break;
    case kInvalidFloat:
      SetLineString("-ERR value is not a valid float");
      break;
    case kOverFlow:
      SetLineString("-ERR increment or decrement would overflow");
      break;
    case kNotFound:
      SetLineString("-ERR no such key");
      break;
    case kOutOfRange:
      SetLineString("-ERR index out of range");
      break;
    case kInvalidPwd:
      SetLineString("-ERR invalid password");
      break;
    case kNoneBgsave:
      SetLineString("-ERR No BGSave Works now");
      break;
    case kPurgeExist:
      SetLineString("-ERR binlog already in purging...");
      break;
    case kInvalidParameter:
      SetLineString("-ERR Invalid Argument");
      break;
    case kWrongNum:
      AppendStringRaw("-ERR wrong number of arguments for '");
      AppendStringRaw(content);
      AppendStringRaw("' command\r\n");
      break;
    case kInvalidIndex:
      AppendStringRaw("-ERR invalid DB index for '");
      AppendStringRaw(content);
      AppendStringRaw("'\r\n");
      break;
    case kInvalidDbType:
      AppendStringRaw("-ERR invalid DB for '");
      AppendStringRaw(content);
      AppendStringRaw("'\r\n");
      break;
    case kInconsistentHashTag:
      SetLineString("-ERR parameters hashtag is inconsistent");
    case kInvalidDB:
      AppendStringRaw("-ERR invalid DB for '");
      AppendStringRaw(content);
      AppendStringRaw("'\r\n");
      break;
    case kErrOther:
      AppendStringRaw("-ERR ");
      AppendStringRaw(content);
      AppendStringRaw(CRLF);
      break;
    case KIncrByOverFlow:
      AppendStringRaw("-ERR increment would produce NaN or Infinity");
      AppendStringRaw(content);
      AppendStringRaw(CRLF);
      break;
    default:
      break;
  }
}
CmdRes::~CmdRes() { message_.clear(); }

thread_local PClient* PClient::s_current = nullptr;

std::mutex monitors_mutex;
std::set<std::weak_ptr<PClient>, std::owner_less<std::weak_ptr<PClient> > > monitors;

void PClient::SetSubCmdName(const std::string& name) {
  subCmdName_ = name;
  std::transform(subCmdName_.begin(), subCmdName_.end(), subCmdName_.begin(), ::tolower);
}

std::string PClient::FullCmdName() const {
  if (subCmdName_.empty()) {
    return cmdName_;
  }
  return cmdName_ + "|" + subCmdName_;
}

int PClient::processInlineCmd(const char* buf, size_t bytes, std::vector<std::string>& params) {
  if (bytes < 2) {
    return 0;
  }

  std::string res;

  for (size_t i = 0; i + 1 < bytes; ++i) {
    if (buf[i] == '\r' && buf[i + 1] == '\n') {
      if (!res.empty()) {
        params.emplace_back(std::move(res));
      }

      return static_cast<int>(i + 2);
    }

    if (isblank(buf[i])) {
      if (!res.empty()) {
        params.reserve(4);
        params.emplace_back(std::move(res));
      }
    } else {
      res.reserve(16);
      res.push_back(buf[i]);
    }
  }

  return 0;
}

static int ProcessMaster(const char* start, const char* end) {
  auto state = PREPL.GetMasterState();

  switch (state) {
    case PReplState_connected:
      // discard all requests before sync;
      // or continue serve with old data? TODO
      return static_cast<int>(end - start);

    case PReplState_wait_auth:
      if (end - start >= 5) {
        if (strncasecmp(start, "+OK\r\n", 5) == 0) {
          PClient::Current()->SetAuth();
          return 5;
        } else {
          assert(!!!"check masterauth config, master password maybe wrong");
        }
      } else {
        return 0;
      }
      break;

    case PReplState_wait_replconf:
      if (end - start >= 5) {
        if (strncasecmp(start, "+OK\r\n", 5) == 0) {
          return 5;
        } else {
          assert(!!!"check error: send replconf command");
        }
      } else {
        return 0;
      }
      break;

    case PReplState_wait_rdb: {
      const char* ptr = start;
      // recv RDB file
      if (PREPL.GetRdbSize() == static_cast<std::size_t>(-1)) {
        ++ptr;  // skip $
        int s;
        if (PParseResult::ok == GetIntUntilCRLF(ptr, end - ptr, s)) {
          assert(s > 0);  // check error for your masterauth or master config

          PREPL.SetRdbSize(s);
          INFO("recv rdb size {}", s);
        }
      } else {
        auto rdb = static_cast<std::size_t>(end - ptr);
        PREPL.SaveTmpRdb(ptr, rdb);
        ptr += rdb;
      }

      return static_cast<int>(ptr - start);
    }

    case PReplState_online:
      break;

    default:
      assert(!!!"wrong master state");
  }

  return -1;  // do nothing
}

int PClient::handlePacket(const char* start, int bytes) {
  auto conn = getTcpConnection();
  if (!conn) {
    ERROR("BUG: conn can't be null when recv data");
    return -1;
  }

  s_current = this;

  const char* const end = start + bytes;
  const char* ptr = start;

  if (isPeerMaster()) {
    //  check slave state
    auto recved = ProcessMaster(start, end);
    if (recved != -1) {
      return recved;
    }
  }

  auto parseRet = parser_.ParseRequest(ptr, end);
  if (parseRet == PParseResult::error) {
    if (!parser_.IsInitialState()) {
      conn->ActiveClose();
      return 0;
    }

    // try inline command
    std::vector<std::string> params;
    auto len = processInlineCmd(ptr, bytes, params);
    if (len == 0) {
      return 0;
    }

    ptr += len;
    parser_.SetParams(params);
    parseRet = PParseResult::ok;
  } else if (parseRet != PParseResult::ok) {
    return static_cast<int>(ptr - start);
  }

  DEFER { reset(); };

  // handle packet
  //  const auto& params = parser_.GetParams();
  if (params_.empty()) {
    return static_cast<int>(ptr - start);
  }

  params_ = parser_.GetParams();
  if (params_.empty()) {
    return static_cast<int>(ptr - start);
  }

  argv_ = params_;
  cmdName_ = params_[0];
  pstd::StringToLower(cmdName_);

  if (!auth_) {
    if (cmdName_ == kCmdNameAuth) {
      auto now = ::time(nullptr);
      if (now <= last_auth_ + 1) {
        // avoid guess password.
        conn->ActiveClose();
        return 0;
      } else {
        last_auth_ = now;
      }
    } else {
      SetLineString("-NOAUTH Authentication required.");
      return static_cast<int>(ptr - start);
    }
  }

  DEBUG("client {}, cmd {}", conn->GetUniqueId(), cmdName_);

  PSTORE.SelectDB(db_);
  FeedMonitors(params_);

  //  const PCommandInfo* info = PCommandTable::GetCommandInfo(cmdName_);

  //  if (!info) {  // 如果这个命令不存在，那么就走新的命令处理流程
  executeCommand();
  //    return static_cast<int>(ptr - start);
  //  }

  // check transaction
  //  if (IsFlagOn(ClientFlag_multi)) {
  //    if (cmdName_ != kCmdNameMulti && cmdName_ != kCmdNameExec && cmdName_ != kCmdNameWatch &&
  //        cmdName_ != kCmdNameUnwatch && cmdName_ != kCmdNameDiscard) {
  //      if (!info->CheckParamsCount(static_cast<int>(params.size()))) {
  //        ERROR("queue failed: cmd {} has params {}", cmdName_, params.size());
  //        ReplyError(info ? PError_param : PError_unknowCmd, &reply_);
  //        FlagExecWrong();
  //      } else {
  //        if (!IsFlagOn(ClientFlag_wrongExec)) {
  //          queue_cmds_.push_back(params);
  //        }
  //
  //        reply_.PushData("+QUEUED\r\n", 9);
  //        INFO("queue cmd {}", cmdName_);
  //      }
  //
  //      return static_cast<int>(ptr - start);
  //    }
  //  }

  // check readonly slave and execute command
  //  PError err = PError_ok;
  //  if (PREPL.GetMasterState() != PReplState_none && !IsFlagOn(ClientFlag_master) &&
  //      (info->attr & PCommandAttr::PAttr_write)) {
  //    err = PError_readonlySlave;
  //    ReplyError(err, &reply_);
  //  } else {
  //    PSlowLog::Instance().Begin();
  //    err = PCommandTable::ExecuteCmd(params, info, IsFlagOn(ClientFlag_master) ? nullptr : &reply_);
  //    PSlowLog::Instance().EndAndStat(params);
  //  }
  //
  //  if (err == PError_ok && (info->attr & PAttr_write)) {
  //    Propagate(params);
  //  }

  return static_cast<int>(ptr - start);
}

// 为了兼容老的命令处理流程，新的命令处理流程在这里
// 后面可以把client这个类重构，完整的支持新的命令处理流程
void PClient::executeCommand() {
  auto [cmdPtr, ret] = g_pikiwidb->GetCmdTableManager().GetCommand(CmdName(), this);

  if (!cmdPtr) {
    if (ret == CmdRes::kInvalidParameter) {
      SetRes(CmdRes::kInvalidParameter);
    } else {
      SetRes(CmdRes::kSyntaxErr, "unknown command '" + CmdName() + "'");
    }
    return;
  }

  if (!cmdPtr->CheckArg(params_.size())) {
    SetRes(CmdRes::kSyntaxErr, "wrong number of arguments for '" + CmdName() + "' command");
    return;
  }

  // execute a specific command
  cmdPtr->Execute(this);
}

PClient* PClient::Current() { return s_current; }

PClient::PClient(TcpConnection* obj)
    : tcp_connection_(std::static_pointer_cast<TcpConnection>(obj->shared_from_this())),
      db_(0),
      flag_(0),
      name_("clientxxx"),
      parser_(params_) {
  auth_ = false;
  SelectDB(0);
  reset();
}

int PClient::HandlePackets(pikiwidb::TcpConnection* obj, const char* start, int size) {
  int total = 0;

  while (total < size) {
    auto processed = handlePacket(start + total, size - total);
    if (processed <= 0) {
      break;
    }

    total += processed;
  }

  obj->SendPacket(Message());
  Clear();
  //  reply_.Clear();
  return total;
}

void PClient::OnConnect() {
  if (isPeerMaster()) {
    PREPL.SetMasterState(PReplState_connected);
    PREPL.SetMaster(std::static_pointer_cast<PClient>(shared_from_this()));

    SetName("MasterConnection");
    SetFlag(ClientFlagMaster);

    if (g_config.masterauth.empty()) {
      SetAuth();
    }
  } else {
    if (g_config.password.empty()) {
      SetAuth();
    }
  }
}

const std::string& PClient::PeerIP() const {
  if (auto c = getTcpConnection(); c) {
    return c->GetPeerIp();
  }

  static const std::string kEmpty;
  return kEmpty;
}

int PClient::PeerPort() const {
  if (auto c = getTcpConnection(); c) {
    return c->GetPeerPort();
  }

  return -1;
}

bool PClient::SendPacket(const std::string& buf) {
  if (auto c = getTcpConnection(); c) {
    return c->SendPacket(buf);
  }

  return false;
}

bool PClient::SendPacket(const void* data, size_t size) {
  if (auto c = getTcpConnection(); c) {
    return c->SendPacket(data, size);
  }

  return false;
}
bool PClient::SendPacket(UnboundedBuffer& data) {
  if (auto c = getTcpConnection(); c) {
    return c->SendPacket(data);
  }

  return false;
}

bool PClient::SendPacket(const evbuffer_iovec* iovecs, size_t nvecs) {
  if (auto c = getTcpConnection(); c) {
    return c->SendPacket(iovecs, nvecs);
  }

  return false;
}

void PClient::Close() {
  if (auto c = getTcpConnection(); c) {
    c->ActiveClose();
    tcp_connection_.reset();
  }
}

bool PClient::SelectDB(int db) {
  if (PSTORE.SelectDB(db) >= 0) {
    db_ = db;
    return true;
  }

  return false;
}

void PClient::reset() {
  s_current = nullptr;
  parser_.Reset();
}

bool PClient::isPeerMaster() const {
  const auto& repl_addr = PREPL.GetMasterAddr();
  return repl_addr.GetIP() == PeerIP() && repl_addr.GetPort() == PeerPort();
}

int PClient::uniqueID() const {
  if (auto c = getTcpConnection(); c) {
    return c->GetUniqueId();
  }

  return -1;
}

bool PClient::Watch(int dbno, const std::string& key) {
  DEBUG("Client {} watch {}, db {}", name_, key, dbno);
  return watch_keys_[dbno].insert(key).second;
}

bool PClient::NotifyDirty(int dbno, const std::string& key) {
  if (IsFlagOn(ClientFlagDirty)) {
    INFO("client is already dirty {}", uniqueID());
    return true;
  }

  if (watch_keys_[dbno].contains(key)) {
    INFO("{} client become dirty because key {} in db {}", uniqueID(), key, dbno);
    SetFlag(ClientFlagDirty);
    return true;
  } else {
    INFO("Dirty key is not exist: {}, because client unwatch before dirty", key);
  }

  return false;
}

bool PClient::Exec() {
  DEFER {
    this->ClearMulti();
    this->ClearWatch();
  };

  if (IsFlagOn(ClientFlagWrongExec)) {
    return false;
  }

  if (IsFlagOn(ClientFlagDirty)) {
    //    FormatNullArray(&reply_);
    AppendString("");
    return true;
  }

  //  PreFormatMultiBulk(queue_cmds_.size(), &reply_);
  //  for (const auto& cmd : queue_cmds_) {
  //    DEBUG("EXEC {}, for client {}", cmd[0], UniqueId());
  //    const PCommandInfo* info = PCommandTable::GetCommandInfo(cmd[0]);
  //    PError err = PCommandTable::ExecuteCmd(cmd, info, &reply_);

  // may dirty clients;
  //    if (err == PError_ok && (info->attr & PAttr_write)) {
  //      Propagate(cmd);
  //    }
  //  }

  return true;
}

void PClient::ClearMulti() {
  queue_cmds_.clear();
  ClearFlag(ClientFlagMulti);
  ClearFlag(ClientFlagWrongExec);
}

void PClient::ClearWatch() {
  watch_keys_.clear();
  ClearFlag(ClientFlagDirty);
}

bool PClient::WaitFor(const std::string& key, const std::string* target) {
  bool succ = waiting_keys_.insert(key).second;

  if (succ && target) {
    if (!target_.empty()) {
      ERROR("Wait failed for key {}, because old target {}", key, target_);
      waiting_keys_.erase(key);
      return false;
    }

    target_ = *target;
  }

  return succ;
}

void PClient::SetSlaveInfo() { slave_info_ = std::make_unique<PSlaveInfo>(); }

void PClient::TransferToSlaveThreads() {
  // transfer to slave
  auto tcp_connection = getTcpConnection();
  if (!tcp_connection) {
    return;
  }

  auto loop = tcp_connection->GetEventLoop();
  auto loop_name = loop->GetName();
  if (loop_name.find("slave") == std::string::npos) {
    auto slave_loop = tcp_connection->SelectSlaveEventLoop();
    auto id = tcp_connection->GetUniqueId();
    auto event_object = loop->GetEventObject(id);
    loop->Unregister(event_object);
    event_object->SetUniqueId(-1);
    slave_loop->Register(event_object, 0);
    tcp_connection->ResetEventLoop(slave_loop);
  }
}

void PClient::AddCurrentToMonitor() {
  std::unique_lock<std::mutex> guard(monitors_mutex);
  monitors.insert(std::static_pointer_cast<PClient>(s_current->shared_from_this()));
}

void PClient::FeedMonitors(const std::vector<std::string>& params) {
  assert(!params.empty());

  {
    std::unique_lock<std::mutex> guard(monitors_mutex);
    if (monitors.empty()) {
      return;
    }
  }

  char buf[512];
  int n = snprintf(buf, sizeof buf, "+[db%d %s:%d]: \"", PSTORE.GetDB(), s_current->PeerIP().c_str(),
                   s_current->PeerPort());

  assert(n > 0);

  for (const auto& e : params) {
    if (n < static_cast<int>(sizeof buf)) {
      n += snprintf(buf + n, sizeof buf - n, "%s ", e.data());
    } else {
      break;
    }
  }

  --n;  // no space follow last param

  {
    std::unique_lock<std::mutex> guard(monitors_mutex);

    for (auto it(monitors.begin()); it != monitors.end();) {
      auto m = it->lock();
      if (m) {
        m->SendPacket(buf, n);
        m->SendPacket("\"" CRLF, 3);

        ++it;
      } else {
        monitors.erase(it++);
      }
    }
  }
}
void PClient::SetKey(std::vector<std::string>& names) {
  keys_ = std::move(names);  // use std::move clear copy expense
}

}  // namespace pikiwidb
