/*
 * Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once
#include "base_cmd.h"

namespace pikiwidb {

class SIsMemberCmd : public BaseCmd {
 public:
  SIsMemberCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class SAddCmd : public BaseCmd {
 public:
  SAddCmd(const std::string &name, int16_t arity);
 public:
  SAddCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;
 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
 private:
  void DoCmd(PClient *client) override;
};

class SUnionStoreCmd : public BaseCmd {
 public:
  SUnionStoreCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class SRemCmd : public BaseCmd {
 public:
  SRemCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class SUnionCmd : public BaseCmd {
 public:
  SUnionCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};
class SInterCmd : public BaseCmd {
 public:
  SInterCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};

class SInterStoreCmd : public BaseCmd {
 public:
  SInterStoreCmd(const std::string &name, int16_t arity);

 protected:
  bool DoInitial(PClient *client) override;

 private:
  void DoCmd(PClient *client) override;
};
}  // namespace pikiwidb
