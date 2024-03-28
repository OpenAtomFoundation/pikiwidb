/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "client.h"
#include "store.h"

namespace pikiwidb {

// command definition

// key cmd
const std::string kCmdNameDel = "del";
const std::string kCmdNameExists = "exists";
const std::string kCmdNamePExpire = "pexpire";
const std::string kCmdNameExpireat = "expireat";
const std::string kCmdNamePExpireat = "pexpireat";
const std::string kCmdNamePersist = "persist";
const std::string kCmdNameKeys = "keys";

const std::string kCmdNamePttl = "pttl";
// string cmd
const std::string kCmdNameSet = "set";
const std::string kCmdNameGet = "get";
const std::string kCmdNameMGet = "mget";
const std::string kCmdNameMSet = "mset";
const std::string kCmdNameGetSet = "getset";
const std::string kCmdNameSetNX = "setnx";
const std::string kCmdNameAppend = "append";
const std::string kCmdNameIncrby = "incrby";
const std::string kCmdNameDecrby = "decrby";
const std::string kCmdNameIncrbyFloat = "incrbyfloat";
const std::string kCmdNameStrlen = "strlen";
const std::string kCmdNameSetBit = "setbit";
const std::string kCmdNameSetEx = "setex";
const std::string kCmdNamePSetEx = "psetex";
const std::string kCmdNameBitOp = "bitop";
const std::string kCmdNameGetBit = "getbit";
const std::string kCmdNameBitCount = "bitcount";
const std::string kCmdNameGetRange = "getrange";
const std::string kCmdNameSetRange = "setrange";
const std::string kCmdNameDecr = "decr";
const std::string kCmdNameIncr = "incr";

// multi
const std::string kCmdNameMulti = "multi";
const std::string kCmdNameExec = "exec";
const std::string kCmdNameWatch = "watch";
const std::string kCmdNameUnwatch = "unwatch";
const std::string kCmdNameDiscard = "discard";

// admin
const std::string kCmdNameConfig = "config";
const std::string kCmdNameFlushdb = "flushdb";
const std::string kCmdNameFlushall = "flushall";
const std::string kCmdNameAuth = "auth";
const std::string kCmdNameSelect = "select";

// hash cmd
const std::string kCmdNameHSet = "hset";
const std::string kCmdNameHGet = "hget";
const std::string kCmdNameHDel = "hdel";
const std::string kCmdNameHMSet = "hmset";
const std::string kCmdNameHMGet = "hmget";
const std::string kCmdNameHGetAll = "hgetall";
const std::string kCmdNameHKeys = "hkeys";
const std::string kCmdNameHLen = "hlen";
const std::string kCmdNameHStrLen = "hstrlen";
const std::string kCmdNameHScan = "hscan";
const std::string kCmdNameHVals = "hvals";
const std::string kCmdNameHIncrbyFloat = "hincrbyfloat";
const std::string kCmdNameHSetNX = "hsetnx";
const std::string kCmdNameHIncrby = "hincrby";
const std::string kCmdNameHRandField = "hrandfield";

// set cmd
const std::string kCmdNameSIsMember = "sismember";
const std::string kCmdNameSAdd = "sadd";
const std::string kCmdNameSUnionStore = "sunionstore";
const std::string kCmdNameSInter = "sinter";
const std::string kCmdNameSRem = "srem";
const std::string kCmdNameSInterStore = "sinterstore";
const std::string kCmdNameSUnion = "sunion";
const std::string kCmdNameSCard = "scard";
const std::string kCmdNameSMove = "smove";
const std::string kCmdNameSRandMember = "srandmember";
const std::string kCmdNameSPop = "spop";
const std::string kCmdNameSMembers = "smembers";
const std::string kCmdNameSDiff = "sdiff";
const std::string kCmdNameSDiffstore = "sdiffstore";

// list cmd
const std::string kCmdNameLPush = "lpush";
const std::string kCmdNameLPushx = "lpushx";
const std::string kCmdNameRPush = "rpush";
const std::string kCmdNameRPushx = "rpushx";
const std::string kCmdNameLPop = "lpop";
const std::string kCmdNameRPop = "rpop";
const std::string kCmdNameLRem = "lrem";
const std::string kCmdNameLRange = "lrange";
const std::string kCmdNameLTrim = "ltrim";
const std::string kCmdNameLSet = "lset";
const std::string kCmdNameLInsert = "linsert";
const std::string kCmdNameLIndex = "lindex";
const std::string kCmdNameLLen = "llen";

// zset cmd
const std::string kCmdNameZAdd = "zadd";
const std::string kCmdNameZRevrange = "zrevrange";
const std::string kCmdNameZRangebyscore = "zrangebyscore";
const std::string kCmdNameZRemrangebyscore = "zremrangebyscore";
const std::string kCmdNameZRemrangebyrank = "zremrangebyrank";
const std::string kCmdNameZRevrangebyscore = "zrevrangebyscore";
const std::string kCmdNameZCard = "zcard";
const std::string kCmdNameZScore = "zscore";
const std::string kCmdNameZRange = "zrange";
const std::string kCmdNameZRangebylex = "zrangebylex";
const std::string kCmdNameZRevrangebylex = "zrevrangebylex";

enum CmdFlags {
  kCmdFlagsWrite = (1 << 0),             // May modify the dataset
  kCmdFlagsReadonly = (1 << 1),          // Doesn't modify the dataset
  kCmdFlagsModule = (1 << 2),            // Implemented by a module
  kCmdFlagsAdmin = (1 << 3),             // Administrative command
  kCmdFlagsPubsub = (1 << 4),            // Pub/Sub related command
  kCmdFlagsNoscript = (1 << 5),          // Not allowed in Lua scripts
  kCmdFlagsBlocking = (1 << 6),          // May block the server
  kCmdFlagsSkipMonitor = (1 << 7),       // Don't propagate to MONITOR
  kCmdFlagsSkipSlowlog = (1 << 8),       // Don't log to slowlog
  kCmdFlagsFast = (1 << 9),              // Tagged as fast by developer
  kCmdFlagsNoAuth = (1 << 10),           // Skip ACL checks
  kCmdFlagsMayReplicate = (1 << 11),     // May replicate even if writes are disabled
  kCmdFlagsProtected = (1 << 12),        // Don't accept in scripts
  kCmdFlagsModuleNoCluster = (1 << 13),  // No cluster mode support
  kCmdFlagsNoMulti = (1 << 14),          // Cannot be pipelined
  kCmdFlagsExclusive = (1 << 15),        // May change Storage pointer, like pika's kCmdFlagsSuspend
};

enum AclCategory {
  kAclCategoryKeyspace = (1 << 0),
  kAclCategoryRead = (1 << 1),
  kAclCategoryWrite = (1 << 2),
  kAclCategorySet = (1 << 3),
  kAclCategorySortedSet = (1 << 4),
  kAclCategoryList = (1 << 5),
  kAclCategoryHash = (1 << 6),
  kAclCategoryString = (1 << 7),
  kAclCategoryBitmap = (1 << 8),
  kAclCategoryHyperloglog = (1 << 9),
  kAclCategoryGeo = (1 << 10),
  kAclCategoryStream = (1 << 11),
  kAclCategoryPubsub = (1 << 12),
  kAclCategoryAdmin = (1 << 13),
  kAclCategoryFast = (1 << 14),
  kAclCategorySlow = (1 << 15),
  kAclCategoryBlocking = (1 << 16),
  kAclCategoryDangerous = (1 << 17),
  kAclCategoryConnection = (1 << 18),
  kAclCategoryTransaction = (1 << 19),
  kAclCategoryScripting = (1 << 20)
};

/**
 * @brief Base class for all commands
 * BaseCmd, as the base class for all commands, mainly implements some common functions
 * such as command name, number of parameters, command flag
 * All data related to a single command execution cannot be defined in Base Cmd and its derived classes.
 * Because the command may be executed in multiple threads at the same time, the data defined in the command
 * will be overwritten by other threads, causing the command to be executed incorrectly.
 * Therefore, the data related to the execution of the command must be defined in the `CmdContext` class.
 * The `CmdContext` class is passed to the command for execution.
 * Base Cmd and its derived classes only provide corresponding functions and logical processing for command execution,
 * but do not provide data storage.
 *
 * This avoids creating a new object every time a command is executed and reduces memory allocation
 * But some data that does not change during command execution
 * (data that does not need to be changed after command initialization) can be placed in Base Cmd
 * For example: command name, number of parameters, command flag, etc.
 */
class BaseCmd : public std::enable_shared_from_this<BaseCmd> {
 public:
  // 这些感觉不需要了

  //  enum CmdStage { kNone, kBinlogStage, kExecuteStage };
  //  struct HintKeys {
  //    HintKeys() = default;
  //    void Push(const std::string& key, int hint) {
  //      keys.push_back(key);
  //      hints.push_back(hint);
  //    }
  //    bool empty() const { return keys.empty() && hints.empty(); }
  //    std::vector<std::string> keys;
  //    std::vector<int> hints;
  //  };

  //  struct CommandStatistics {
  //    CommandStatistics() = default;
  //    CommandStatistics(const CommandStatistics& other) {
  //      cmd_time_consuming.store(other.cmd_time_consuming.load());
  //      cmd_count.store(other.cmd_count.load());
  //    }
  //    std::atomic<int32_t> cmd_count = {0};
  //    std::atomic<int32_t> cmd_time_consuming = {0};
  //  };
  //  CommandStatistics state;

  /**
   * @brief Construct a new Base Cmd object
   * @param name command name
   * @param arity number of parameters
   * @param flag command flag
   * @param aclCategory command acl category
   */
  BaseCmd(std::string name, int16_t arity, uint32_t flag, uint32_t aclCategory);
  virtual ~BaseCmd() = default;

  // check that each parameter meets the requirements
  bool CheckArg(size_t num) const;

  // get the key in the current command
  // e.g: set myKey value, return myKey
  std::vector<std::string> CurrentKey(PClient* client) const;

  // the entry point for the entire cmd execution
  // 后续如果需要拓展，在这个函数里面拓展
  // 对外部调用者来说，只暴露这个函数，其他的都是内部实现
  void Execute(PClient* client);

  // binlog 相关的函数，我对这块不熟悉，就没有移植，后面binlog应该可以在Execute里面调用
  virtual std::string ToBinlog(uint32_t exec_time, uint32_t term_id, uint64_t logic_id, uint32_t filenum,
                               uint64_t offset);
  virtual void DoBinlog();

  //  virtual void ProcessFlushDBCmd();
  //  virtual void ProcessFlushAllCmd();
  //  virtual void ProcessSingleSlotCmd();
  //  virtual void ProcessMultiSlotCmd();
  //  virtual void ProcessDoNotSpecifySlotCmd();
  //  virtual void Do(std::shared_ptr<Slot> slot = nullptr) = 0;
  //  virtual Cmd* Clone() = 0;
  //  used for execute multikey command into different slots
  //  virtual void Split(std::shared_ptr<Slot> slot, const HintKeys& hint_keys) = 0;
  //  virtual void Merge() = 0;

  bool HasFlag(uint32_t flag) const;
  void SetFlag(uint32_t flag);
  void ResetFlag(uint32_t flag);

  // Processing functions related to subcommands. If this command is not a subcommand,
  // then these functions do not need to be implemented.
  // If it is a subcommand, you need to implement these functions
  // e.g: CmdConfig is a subcommand, and the subcommand is set and get
  virtual bool HasSubCommand() const;  // The command is there a sub command
  virtual BaseCmd* GetSubCmd(const std::string& cmdName);

  uint32_t AclCategory() const;
  void AddAclCategory(uint32_t aclCategory);
  std::string Name() const;
  //  CmdRes& Res();
  //  std::string db_name() const;
  //  BinlogOffset binlog_offset() const;
  //  PikaCmdArgsType& argv();

  //  void SetConn(const std::shared_ptr<net::NetConn>& conn);
  //  std::shared_ptr<net::NetConn> GetConn();

  //  void SetResp(const std::shared_ptr<std::string>& resp);
  //  std::shared_ptr<std::string> GetResp();

  uint32_t GetCmdId() const;

  bool isExclusive() { return static_cast<bool>(flag_ & kCmdFlagsExclusive); }

 protected:
  // Execute a specific command
  virtual void DoCmd(PClient* client) = 0;

  std::string name_;
  int16_t arity_ = 0;
  uint32_t flag_ = 0;

  //  CmdRes res_;
  //  std::string dbName_;
  //  std::weak_ptr<net::NetConn> conn_;
  //  std::weak_ptr<std::string> resp_;
  //  uint64_t doDuration_ = 0;

  uint32_t cmdId_ = 0;
  uint32_t aclCategory_ = 0;

 private:
  // The function to be executed first before executing `DoCmd`
  // What needs to be done at present are: extract the key in the command and fill it into the context
  // If this function returns false, then Do Cmd will not be executed
  virtual bool DoInitial(PClient* client) = 0;

  //  virtual void Clear(){};
  //  BaseCmd& operator=(const BaseCmd&);
};

class BaseCmdGroup : public BaseCmd {
 public:
  BaseCmdGroup(const std::string& name, uint32_t flag);
  BaseCmdGroup(const std::string& name, int16_t arity, uint32_t flag);

  ~BaseCmdGroup() override = default;

  void AddSubCmd(std::unique_ptr<BaseCmd> cmd);
  BaseCmd* GetSubCmd(const std::string& cmdName) override;

  // group cmd this function will not be called
  void DoCmd(PClient* client) override{};

  // group cmd this function will not be called
  bool DoInitial(PClient* client) override;

 private:
  std::map<std::string, std::unique_ptr<BaseCmd>> subCmds_;
};
}  // namespace pikiwidb
