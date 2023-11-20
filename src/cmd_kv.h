/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "base_cmd.h"

enum BitOp_op {
  BitOp_and,
  BitOp_or,
  BitOp_not,
  BitOp_xor,
};

namespace pikiwidb {

class GetCmd : public BaseCmd {
 public:
  GetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class SetCmd : public BaseCmd {
 public:
  SetCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class BitOp : public BaseCmd {
 public:
  BitOp(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

}  // namespace pikiwidb
