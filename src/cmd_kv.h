/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "base_cmd.h"

namespace pikiwidb {

class GetCmd : public BaseCmd {
 public:
  GetCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
  void ReadCache(PClient *client) override;
};

class SetCmd : public BaseCmd {
 public:
  SetCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class BitOpCmd : public BaseCmd {
 public:
  enum BitOp {
    kBitOpAnd,
    kBitOpOr,
    kBitOpNot,
    kBitOpXor,
  };
  BitOpCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class StrlenCmd : public BaseCmd {
 public:
  StrlenCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
  void ReadCache(PClient *client) override;
};

class SetExCmd : public BaseCmd {
 public:
  SetExCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class PSetExCmd : public BaseCmd {
 public:
  PSetExCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class SetNXCmd : public BaseCmd {
 public:
  SetNXCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class AppendCmd : public BaseCmd {
 public:
  AppendCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class GetSetCmd : public BaseCmd {
 public:
  GetSetCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class MGetCmd : public BaseCmd {
 public:
  MGetCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
  void ReadCache(PClient *client) override;
};

class MSetCmd : public BaseCmd {
 public:
  MSetCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class BitCountCmd : public BaseCmd {
 public:
  BitCountCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
  void ReadCache(PClient *client) override;
};

class IncrbyCmd : public BaseCmd {
 public:
  IncrbyCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

class GetBitCmd : public BaseCmd {
 public:
  GetBitCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
  void ReadCache(PClient *client) override;
};

class IncrbyFloatCmd : public BaseCmd {
 public:
  IncrbyFloatCmd(const std::string &name, int16_t arity, uint32_t flag);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
  void DoThroughDB(PClient *client) override;
  void DoUpdateCache(PClient *client) override;
};

}  // namespace pikiwidb
