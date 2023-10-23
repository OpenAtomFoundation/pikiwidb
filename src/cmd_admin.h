/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "base_cmd.h"
#include "cmd_context.h"

namespace pikiwidb {

class CmdConfig : public BaseCmdGroup {
 public:
  CmdConfig(const std::string& name, int arity);

  bool HasSubCommand() const override;

 protected:
  bool DoInitial(CmdContext& ctx) override { return true; };

 private:
  std::vector<std::string> subCmd_;

  void DoCmd(CmdContext& ctx) override{};
};

class CmdConfigGet : public BaseCmd {
 public:
  CmdConfigGet(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(CmdContext& ctx) override;

 private:
  void DoCmd(pikiwidb::CmdContext& ctx) override;
};

class CmdConfigSet : public BaseCmd {
 public:
  CmdConfigSet(const std::string& name, int16_t arity);

 protected:
  bool DoInitial(CmdContext& ctx) override;

 private:
  void DoCmd(pikiwidb::CmdContext& ctx) override;
};

}  // namespace pikiwidb
